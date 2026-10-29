#!/usr/bin/env bash
# Builds (if needed) and runs DawIt in this terminal, so its log output shows here.
#
# Usage: ./run.sh [release|debug] [--no-build] [-- app arguments...]
#   release|debug  build configuration (default: release)
#   --no-build     run the existing build without rebuilding
#   --             everything after this is passed to the app, e.g. -- --demo
#
# Only one DawIt can run at a time; quit the running one first.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG=Release
BUILD=1

usage() { sed -n '2,10s/^# \{0,1\}//p' "${BASH_SOURCE[0]}"; }

while [[ $# -gt 0 ]]; do
    case "$1" in
        release|Release) CONFIG=Release ;;
        debug|Debug)     CONFIG=Debug ;;
        --no-build)      BUILD=0 ;;
        -h|--help)       usage; exit 0 ;;
        --)              shift; break ;;
        *)               echo "Unknown option: $1" >&2; usage >&2; exit 1 ;;
    esac
    shift
done

if [[ $BUILD == 1 ]]; then
    "$ROOT/build.sh" "$CONFIG"
fi

ARTEFACTS="$ROOT/build/$CONFIG/DawIt_artefacts/$CONFIG"

case "$(uname -s)" in
    Darwin)               EXE="$ARTEFACTS/DawIt.app/Contents/MacOS/DawIt" ;;
    MINGW*|MSYS*|CYGWIN*) EXE="$ARTEFACTS/DawIt.exe" ;;
    *)                    EXE="$ARTEFACTS/DawIt" ;;
esac

if [[ ! -x "$EXE" ]]; then
    echo "No $CONFIG build found at $EXE - run without --no-build first." >&2
    exit 1
fi

exec "$EXE" ${1+"$@"}
