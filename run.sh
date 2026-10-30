#!/usr/bin/env bash
# Builds (if needed) and runs DawIt in this terminal, so its log output shows here.
#
# Usage: ./run.sh [release|debug] [--no-build] [-- app arguments...]
#   release|debug  build configuration (default: release)
#   --no-build     run the existing build without rebuilding
#   --             everything after this is passed to the app, e.g. -- --demo
#
# Only one DawIt can run at a time; quit the running one first. On macOS, quit DawIt itself (Cmd+Q): Ctrl+C
# here stops only the log.

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

# On macOS, DawIt goes through `open` so it runs as its own app. Started straight from a terminal, macOS would
# count the terminal as the one using the microphone, and record silence unless that terminal may use it.
# The batch modes (screenshots, guide export) don't record, so they run directly and keep their exit code.
if [[ "$(uname -s)" == Darwin && " $* " != *" --screenshot"* && " $* " != *" --export-guides "* ]]; then
    LOG_PIPE="$(mktemp -u "${TMPDIR:-/tmp}/dawit-log.XXXXXX")"
    mkfifo "$LOG_PIPE"
    trap 'rm -f "$LOG_PIPE"' EXIT
    cat "$LOG_PIPE" &
    open -n -W --stdout "$LOG_PIPE" --stderr "$LOG_PIPE" "$ARTEFACTS/DawIt.app" --args ${1+"$@"}
    wait
    exit 0
fi

exec "$EXE" ${1+"$@"}
