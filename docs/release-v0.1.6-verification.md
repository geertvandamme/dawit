# v0.1.6 packages, source, notices and dependency manifest verification

Checked 10 November 2026 against tag `v0.1.6`, commit
`2864d0506f18edc33563183b308a2f339a3eef1f`, and the successful
[release workflow](https://github.com/geertvandamme/dawit/actions/runs/42803462111).
The release is a draft, marked as a prerelease. This record says what was
checked and what is still open. It doesn't show that the release is ready to
publish.

Since v0.1.5 the app checks the processor before detecting lyrics (on x64,
AVX2, FMA and F16C), its dialogs say "this computer" instead of "this Mac", and
the draft's notes are filled in from `tools/release-notes.md`. No dependency,
`LICENSE` or `licenses/` file changed: between the two tags only
`.github/workflows/release.yml` (the release notes) and the version in
`CMakeLists.txt` changed among the build and packaging files.

## Packages

- Downloaded all six draft assets and checked each against the draft's
  `SHA256SUMS.txt`: all match. The checksums come from the same release, so
  they check the transfer, not an independent signature.
- The macOS app says version 0.1.6, and its ad-hoc signature passes
  `codesign --verify --deep --strict`. The app isn't notarised.
- The release notes say version 0.1.6 throughout, have no placeholders left,
  link the complete source and name the dependency manifest. Their known issue
  for older PCs now says DawIt explains that lyrics detection isn't available,
  instead of closing.
- The Linux `DawIt` binary holds the new explanation ("isn't available on this
  computer"), and its source link is
  `https://github.com/geertvandamme/dawit/releases/download/v0.1.6/DawIt-0.1.6-complete-source.tar.gz`,
  which works only once the release is published.

## Complete source archive

- `DawIt-0.1.6-complete-source.tar.gz` (419 MB, SHA-256
  `e1c82c7bf1780f4cb8e853585eddf8d8a47bd93fb4f64732c8c06e4e97afa8bf`) holds
  DawIt at the tag and 29 upstream files.
- Running `tools/make-complete-source.py --commit v0.1.6` on a Mac gave a
  byte-identical archive to the one the Linux CI job attached.

## Dependency manifest and build records

- `DawIt-0.1.6-dependencies.json` lists 30 components and 5 app downloads, the
  same as v0.1.5's, and a build record for each platform. Running
  `tools/make-dependency-manifest.py` on the downloaded source archive gives
  the same components, app downloads and system requirements.
- The Linux package's `DawIt` and `libonnxruntime.so.1` match the hashes their
  build record took as built, byte for byte.
- Each binary needs the same libraries as in v0.1.5, on every platform. The
  runner images, compilers and SDKs are the same as v0.1.5's
  ([table](release-v0.1.5-verification.md#dependency-manifest-and-build-records)).

## ONNX Runtime

The as-built hashes of the macOS (`7c041d84…`) and Linux (`cc9bab3c…`)
libraries are again the same as v0.1.4's and v0.1.5's, so the
[v0.1.4 notice audit](release-v0.1.4-onnx-notice-audit.md) covers these exact
libraries. The Windows DLL differs again (`36624dfa…`), as MSVC builds always
do; it needs the same DLLs as before. Each job's `verify-release-onnx.py` step
passed, and the Windows job installed and removed the installer.

## Notices

All 46 files of the tagged `LICENSE` and `licenses/` are present and identical
(after normalising line endings) in the Linux archive, in the DMG beside the
app, and in the app bundle's `Contents/Resources`. The Windows installer
wasn't extracted; the Windows CI job installs it and compares the installed
licence files with the tag, and it passed.

## Still open

- The lyrics processor check on a real x64 PC without AVX2. On this Mac the
  x64 check, run under Rosetta (which reports no AVX), found AVX2, FMA and F16C
  missing, as it should.
- The Windows installer's contents, checked independently of CI.
- Signing, notarisation, the Visual C++ runtime decision, the website's
  download links and clean-machine tests, as listed in `DISTRIBUTION_TODO.md`.
