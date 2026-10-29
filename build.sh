#!/usr/bin/env bash
# Builds DawIt.
#
# Usage: ./build.sh [release|debug] [--test] [--plugins] [--install-plugins] [--clean]
#   release|debug       build configuration (default: release)
#   --test              also build and run the headless engine tests
#   --plugins           also build DawIt's effects and synths as VST3 and AU plugins (slow: each compiles JUCE)
#   --install-plugins   also copy them where DAWs look (~/Library/Audio/Plug-Ins/VST3 and Components); implies --plugins
#   --clean             delete the build directory first
#
# Output goes to build/<Config>/.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG=Release
TESTS=OFF
PLUGINS=OFF
INSTALL_PLUGINS=0
CLEAN=0

usage() { sed -n '2,12s/^# \{0,1\}//p' "${BASH_SOURCE[0]}"; }

for arg in "$@"; do
    case "$arg" in
        release|Release) CONFIG=Release ;;
        debug|Debug)     CONFIG=Debug ;;
        --test)          TESTS=ON ;;
        --plugins)       PLUGINS=ON ;;
        --install-plugins) PLUGINS=ON ; INSTALL_PLUGINS=1 ;;
        --clean)         CLEAN=1 ;;
        -h|--help)       usage; exit 0 ;;
        *)               echo "Unknown option: $arg" >&2; usage >&2; exit 1 ;;
    esac
done

BUILD_DIR="$ROOT/build/$CONFIG"

if [[ $CLEAN == 1 ]]; then
    echo "Removing $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

# Prefer Ninja when available; otherwise let CMake pick the platform default.
if command -v ninja >/dev/null 2>&1; then
    cmake -S "$ROOT" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE="$CONFIG" -DDAWIT_BUILD_TESTS="$TESTS" -DDAWIT_BUILD_PLUGINS="$PLUGINS"
else
    cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$CONFIG" -DDAWIT_BUILD_TESTS="$TESTS" -DDAWIT_BUILD_PLUGINS="$PLUGINS"
fi

cmake --build "$BUILD_DIR" --config "$CONFIG" --parallel

case "$(uname -s)" in
    Darwin)               APP="$BUILD_DIR/DawIt_artefacts/$CONFIG/DawIt.app" ; EXE_SUFFIX="" ;;
    MINGW*|MSYS*|CYGWIN*) APP="$BUILD_DIR/DawIt_artefacts/$CONFIG/DawIt.exe" ; EXE_SUFFIX=".exe" ;;
    *)                    APP="$BUILD_DIR/DawIt_artefacts/$CONFIG/DawIt"     ; EXE_SUFFIX="" ;;
esac

if [[ $TESTS == ON ]]; then
    echo
    echo "Running engine tests"
    "$BUILD_DIR/DawItTests_artefacts/$CONFIG/DawItTests$EXE_SUFFIX"
fi

if [[ $PLUGINS == ON ]]; then
    echo
    echo "Plugins in $BUILD_DIR/DawItPlugin*_artefacts/$CONFIG"
fi

if [[ $INSTALL_PLUGINS == 1 ]]; then
    case "$(uname -s)" in
        Darwin)
            VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
            AU_DIR="$HOME/Library/Audio/Plug-Ins/Components" ;;
        Linux)
            VST3_DIR="$HOME/.vst3"
            AU_DIR="" ;;
        *)
            echo "--install-plugins is for macOS and Linux; copy the .vst3 folders yourself." >&2
            exit 1 ;;
    esac
    mkdir -p "$VST3_DIR"
    for plugin in "$BUILD_DIR"/DawItPlugin*_artefacts/"$CONFIG"/VST3/*.vst3; do
        rm -rf "$VST3_DIR/$(basename "$plugin")"
        cp -R "$plugin" "$VST3_DIR/"
        echo "Installed $(basename "$plugin") in $VST3_DIR"
    done
    if [[ -n "$AU_DIR" ]]; then
        mkdir -p "$AU_DIR"
        for plugin in "$BUILD_DIR"/DawItPlugin*_artefacts/"$CONFIG"/AU/*.component; do
            rm -rf "$AU_DIR/$(basename "$plugin")"
            cp -R "$plugin" "$AU_DIR/"
            echo "Installed $(basename "$plugin") in $AU_DIR"
        done
    fi
fi

echo
echo "Built $APP"
