# Building DawIt from the complete source archive

Every DawIt release comes with `DawIt-<version>-complete-source.tar.gz`. With it you can rebuild that release, change it and install your own version, without a network and without the maintainer's keys. DawIt is licensed under the GNU AGPL v3 (see `LICENSE`), and the archive is how DawIt gives you the source of everything in the app, not only DawIt's own code.

GitHub's *Source code* downloads hold only DawIt's own files. The complete archive also holds every library, model and font that the build downloads.

## What's inside

- **DawIt's own files** at the release commit, exactly as git has them: the source, tests, assets, build and packaging scripts, licences (`LICENSE`, `licenses/`) and DawIt's patches to other projects (`tools/patches/`).
- **`third_party/mirror/`**: the unmodified upstream download of every component the build uses, kept at its URL without `https://`. For example, demucs.cpp is in `third_party/mirror/github.com/sevagh/demucs.cpp/archive/f1206e9….tar.gz`. These include:
  - JUCE 8.0.9 with the VST3 SDK, demucs.cpp, Eigen, whisper.cpp 1.9.5 with ggml, TinySoundFont 0.9 and stb_vorbis 1.22;
  - ONNX Runtime 1.30.0, with the dependencies its build fetches (Abseil, Protobuf, ONNX, RE2, Eigen and others);
  - the Basic Pitch model, the CMU Pronouncing Dictionary and the Bravura font, which are built into the app.
- **`third_party/MANIFEST.json`**: each mirrored file with its URL, its role and its SHA-256 hash, plus the DawIt version and commit.

The ONNX build also uses `protoc`, the Protocol Buffers compiler. The mirror has the prebuilt `protoc` releases for macOS, Linux and Windows. They run only during the build and aren't part of the app, and their source is the Protobuf archive beside them.

**Not inside:** the Demucs and Whisper models and the MuseScore General SoundFont. DawIt doesn't ship them. The app downloads them on first use, after asking.

## How the build finds the files

You don't have to unpack anything by hand. CMake looks for each pinned download in `third_party/mirror/` before it goes to the network (`cmake/DawItPinned.cmake`). The ONNX build scripts in `tools/` do the same for ONNX Runtime and its dependencies. Every file is still checked against the hash pinned in the source, so a changed file stops the build.

To keep the mirror somewhere else, set `DAWIT_MIRROR_DIR` for the ONNX scripts and `-DDAWIT_MIRROR_DIR=<folder>` for CMake.

## Rebuilding

You need the tools from the README's *Building* section: CMake 3.22 or later, a C++20 compiler, Git, Python 3 and Ninja (or Visual Studio on Windows). Unpack the archive and open a terminal in its folder:

```bash
tar -xzf DawIt-<version>-complete-source.tar.gz
cd DawIt-<version>-complete-source
```

Git is only used to record the ONNX source locally; nothing is fetched. A build from the archive isn't a git checkout, so the About window says "commit unknown".

### macOS (Apple Silicon)

```bash
DAWIT_ONNX_WORK_DIR="$PWD/onnx" tools/build-onnxruntime-source-macos.sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DDAWIT_BUILD_TESTS=ON -DDAWIT_ORT_SOURCE_BUILD="$PWD/onnx"
cmake --build build
./build/DawItTests_artefacts/Release/DawItTests
```

Building ONNX Runtime takes the longest: about 6 minutes on an Apple Silicon Mac with `DAWIT_ONNX_JOBS=8` (the script uses 4 by default). The app is `build/DawIt_artefacts/Release/DawIt.app`.

### Linux (x86-64)

Install the packages from the README's *Linux* section first, then:

```bash
DAWIT_ONNX_WORK_DIR="$PWD/onnx" tools/build-onnxruntime-source-linux.sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DDAWIT_BUILD_TESTS=ON -DDAWIT_ORT_SOURCE_BUILD="$PWD/onnx"
cmake --build build
./build/DawItTests_artefacts/Release/DawItTests
```

### Windows (x64)

From PowerShell, with Visual Studio 2026 and Python 3. The ONNX script installs the Python package `packaging`, so offline you need to install it beforehand.

```powershell
$env:DAWIT_ONNX_WORK_DIR = "$PWD\onnx"
./tools/build-onnxruntime-source-windows.ps1
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DDAWIT_BUILD_TESTS=ON -DDAWIT_ORT_SOURCE_BUILD="$PWD\onnx"
cmake --build build --config Release
```

## Changing DawIt and installing your version

Change the source, then build again with `cmake --build build`. Run the tests to check nothing broke.

**On macOS**, sign your build ad hoc (no Apple account needed), then copy it to Applications:

```bash
codesign --force --sign - build/DawIt_artefacts/Release/DawIt.app/Contents/Frameworks/libonnxruntime.1.dylib
codesign --force --sign - build/DawIt_artefacts/Release/DawIt.app
ditto build/DawIt_artefacts/Release/DawIt.app /Applications/DawIt.app
```

The first time you open it, macOS may block it. Open *System Settings → Privacy & Security* and choose *Open Anyway*. `tools/package-release-macos.sh <version>` makes a DMG the way the releases are made.

**On Windows**, run `build\DawIt_artefacts\Release\DawIt.exe`, or make an installer with `tools/package-release-windows.ps1` and Inno Setup 6.4.3. **On Linux**, run `build/DawIt_artefacts/Release/DawIt`, or make an archive with `tools/package-release-linux.sh <version>`.

If you give your changed version to others, the AGPL asks you to offer them its source too (sections 5, 6 and 13 of `LICENSE`).

## Making the archive

`tools/make-complete-source.py` makes the archive from a commit, `HEAD` by default:

```bash
tools/make-complete-source.py --commit v0.1.4
```

It reads the pinned downloads from `CMakeLists.txt` (each `dawit_pinned` call) and the ONNX pins from the build scripts. It checks every file's hash and writes `dist/DawIt-<version>-complete-source.tar.gz` with a `.sha256` file beside it. The same commit always gives the same bytes. Downloads are cached in `build/complete-source-cache/`. The release workflow makes and attaches the archive for every tag.

A new pinned download goes through `dawit_pinned` in `CMakeLists.txt`, on one line, with its role in the script's `DAWIT_ROLES`. A new ONNX dependency goes in `ONNX_DEPENDENCIES`.
