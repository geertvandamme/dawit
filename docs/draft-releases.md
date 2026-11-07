# Draft release candidates

The tag workflow in `.github/workflows/release.yml` builds macOS Apple Silicon
and Windows x64, runs the engine tests, and creates a draft GitHub release only
when both jobs succeed. It uploads a DMG, a Windows ZIP and SHA-256 checksums.
The Windows ZIP is a test candidate, not an installer; it may require additional
Microsoft runtime files on a clean machine. The Mac app is ad-hoc signed and
not notarised. Windows binaries are unsigned.

To run it, commit the workflow and packaging scripts, set the CMake project
version, and push a matching tag such as `v0.1.0` from the intended commit. The
workflow rejects a tag that does not match the CMake version. The repository
must allow GitHub Actions to create releases with `contents: write`. Review the
Actions run and the draft under Releases. Rerunning after a draft has been
created will fail at release creation rather than replace its assets.

These drafts are not ready for publication. Complete the applicable checks in
`DISTRIBUTION_TODO.md`, especially the Windows installer and runtime, exact
dependency notices and complete corresponding source, and clean-machine and
download tests. The licence files included with the candidates have not been
verified as a complete binary distribution. Review signing and notarisation
before offering the packages to users. The workflow does not publish a release,
create a complete source archive, or upload signing credentials.
