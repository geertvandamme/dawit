# Draft release candidates

The tag workflow in `.github/workflows/release.yml` builds macOS Apple Silicon
Windows x64 and Linux x64, runs the engine tests, and creates a draft GitHub
release only when all three jobs succeed. It uploads a DMG, a Windows installer,
a Linux tarball and SHA-256 checksums. The Windows per-user installer uses Inno
Setup 6.4.3 and does not install the Microsoft Visual C++ redistributable; the
target may need the x64 runtime installed separately. The Mac app is ad-hoc
signed and not notarised. Windows binaries are unsigned.

The v0.1.2 Mac candidate's bundled ONNX Runtime declares macOS 14.0 as its
minimum, even though that tagged build's CMake deployment target was 11.0. The full app's
minimum system version has not yet been established by testing.

The Linux archive is built on Ubuntu 22.04 and expects compatible system audio,
graphics and networking libraries. Extract it and run `DawIt/DawIt` from the
extracted directory. It is a candidate, not a tested distribution-independent
package; minimum system requirements and clean-machine installation remain open.

To run it, commit the workflow and packaging scripts, set the CMake project
version, and push a matching tag such as `v0.1.0` from the intended commit. The
workflow rejects a tag that does not match the CMake version. The repository
must allow GitHub Actions to create releases with `contents: write`. Review the
Actions run and the draft under Releases. Rerunning after a draft has been
created will fail at release creation rather than replace its assets.

After publication, GitHub records a download count for each release asset. To
inspect counts with the authenticated GitHub CLI, run:

```sh
gh api repos/geertvandamme/dawit/releases --jq '.[] | {tag: .tag_name, assets: [.assets[] | {name, download_count}]}'
```

These drafts are not ready for publication. Complete the applicable checks in
`DISTRIBUTION_TODO.md`, especially installer and runtime validation, exact
dependency notices and complete corresponding source, and clean-machine and
download tests. The licence files included with the candidates have not been
verified as a complete binary distribution. Review signing and notarisation
before offering the packages to users. The workflow does not publish a release,
create a complete source archive, or upload signing credentials.
