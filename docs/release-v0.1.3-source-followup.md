# v0.1.3 source and notice follow-up

This supplements the [v0.1.3 verification](release-v0.1.3-verification.md)
and the [v0.1.2 audit](release-v0.1.2-source-audit.md). It concerns tag
`v0.1.3`, commit `d5ddd5a89a3458c3f4501bbb027be549bddf8b21`, not later
uncommitted work. The findings do not establish licence compliance or mean
that all corresponding source and notices have been identified.

## Additional source evidence

The locally prepared source-evidence collection preserves the tagged DawIt
source, a JUCE archive made from the locally verified 8.0.9 commit, the
hash-checked Demucs, Eigen and whisper.cpp downloads, embedded assets, the
three original ONNX Runtime binary packages, the ONNX Runtime source snapshot
and the pinned vcpkg registry. It also contains the three ONNX Runtime
submodules recorded in the earlier audit and selected transitive sources.
It is labelled **incomplete** and has not been uploaded to the draft release.

Twelve source archives were checked against the SHA-512 values in the ONNX
Runtime source's vcpkg overlay or its pinned vcpkg registry: ONNX 1.22.0,
protobuf 33.6, Abseil 20250814.0, cpuinfo, cpp-client-telemetry 3.10.173.1,
ONNX's separately pinned Eigen, Mbed TLS 3.6.5, curl 8.20.0, SQLite 3.53.2,
zlib 1.3.2, RE2 and nlohmann/json 3.12.0. The collection's
`CHECKED-ONNX-SOURCES.tsv` records the URLs and exact hashes. This proves
that the retrieved source archives match those recipes, not that every
listed input was used, that the list is exhaustive, or that Microsoft's
published binaries were rebuilt from them.

A macOS Apple Silicon app build was also made from a fresh scratch copy of
the collected DawIt and direct dependency sources. Both CMake configuration
and the clean build succeeded with networking explicitly blocked. The scratch
copy redirected the four embedded-asset downloads to collected files and
used CMake source-directory overrides; the tagged source was unchanged. The
build used the collected, prebuilt macOS ONNX Runtime package, not a runtime
rebuilt from source. It did not run the app tests or compare the output bytes
with the CI candidate. The procedure is in `REBUILD-EXPERIMENT.md` in the
local evidence collection. This does not verify Windows or Linux rebuilds.

The shipped Linux ONNX library contains Mbed TLS source paths identifying
`v3.6.5` and contains a `3.53.2` string consistent with the pinned SQLite
port. It also contains libcurl-related strings. The pinned vcpkg registry
specifies curl 8.20.0 and SQLite 3.53.2, but those strings alone do not prove
the exact version or all features linked into the binary. The Windows and
Linux ONNX binaries were previously matched byte-for-byte to the hash-checked
upstream packages; the re-signed macOS dylib's UUID matched the upstream dylib.

GitHub-generated archives do not contain Git submodule contents. The
telemetry SDK's source tree records an optional `lib/modules` submodule at
`c637015fbbe904ed556d27e3b9072f6f2a5ee401`. A request for that revision from
`microsoft/cpp_client_telemetry_modules` returned HTTP 404 with the available
credentials. Its CMake file conditionally includes source from the modules
directory if present. The public source archive omits it; whether it was
present in the upstream release build and whether its contents are needed to
reproduce the distributed libraries have not been established. This 404
does not by itself prove the code was included or that it is unavailable
through every channel. The Mbed TLS framework and the telemetry SDK's
googletest submodules were collected separately; the pinned Mbed TLS port
uses an empty framework placeholder and disables testing.

## Additional notice finding

The pinned curl 8.20.0 source's `COPYING` includes copyright dates 1996–2026;
the curl section in the shipped ONNX aggregate notice has similar permission
text but omits those dates. A verbatim supplementary copy is preserved as
[ONNX-curl-8.20.0-COPYING.txt](release-v0.1.3-notices/ONNX-curl-8.20.0-COPYING.txt)
and in the local evidence collection. Exact curl version and all applicable
file-level notices for the shipped Linux runtime have not been independently
established. This is a conservative item for review, not a conclusion that
the existing distribution violates the licence. The original v0.1.3 packages
have not been changed, and the supplementary file has not been attached to
the draft.

## Still needed

- Establish the actual upstream ONNX Runtime build inputs and modifications
  for each shipped platform, including whether the optional telemetry modules
  were present and any additional source and notices. Microsoft's source
  templates reference an internal asset cache; the actual historical build
  and cache contents have not been obtained.
- Complete the per-platform third-party and file-level notice review, resolve
  the curl notice question, and make the verified notices available to
  recipients in the chosen distribution method.
- Prepare and test a genuinely complete corresponding-source distribution
  from the exact release inputs. The evidence collection includes prebuilt
  ONNX binaries and cannot demonstrate rebuilding ONNX from source. The
  project currently calls for a dedicated archive; that format is a project
  policy, not by itself a universal licence requirement.
- The source and notice work does not replace the separate clean-machine,
  telemetry, compiler-warning and signing checks in the release checklist.
