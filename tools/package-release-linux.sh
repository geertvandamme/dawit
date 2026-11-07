#!/usr/bin/env bash
set -euo pipefail
version=${1:?Pass the project version}
source_dir=${DAWIT_BUILD_DIR:-build}/DawIt_artefacts/Release
output=${DAWIT_DIST_DIR:-dist}
stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
test -f "$source_dir/DawIt"
test -f "$source_dir/libonnxruntime.so.1"
mkdir -p "$stage/DawIt" "$output"
cp -a "$source_dir/." "$stage/DawIt/"
cp LICENSE "$stage/DawIt/LICENSE.txt"
cp -R licenses "$stage/DawIt/licenses"
tar -czf "$output/DawIt-${version}-linux-x64.tar.gz" -C "$stage" DawIt
