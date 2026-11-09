#!/usr/bin/env bash
# Experimental Apple Silicon ONNX Runtime source build for DawIt.
# This does not alter DawIt's normal build or install the resulting library.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="${DAWIT_ONNX_WORK_DIR:-$root/build/onnx-runtime-source-v1.30.0}"
revision=f2c39fe2f838cf35ce7da92824f5a5e3ee6e88a7
changed_file=onnxruntime/core/mlas/lib/qlutgemm.cpp
patch_file="$root/tools/patches/onnxruntime-1.30.0-unique-array.patch"
jobs="${DAWIT_ONNX_JOBS:-4}"

if [[ "$(uname -s)" != Darwin || "$(uname -m)" != arm64 ]]; then
    echo 'This experimental build currently supports Apple Silicon macOS only.' >&2
    exit 1
fi
if [[ ! "$jobs" =~ ^[1-9][0-9]*$ ]]; then
    echo 'DAWIT_ONNX_JOBS must be a positive integer.' >&2
    exit 1
fi
for tool in git cmake ninja python3; do
    command -v "$tool" >/dev/null || { echo "Missing required tool: $tool" >&2; exit 1; }
done

mkdir -p "$work"
work="$(cd "$work" && pwd)"
source_dir="$work/source"
if [[ ! -e "$source_dir" ]]; then
    git init -q "$source_dir"
    git -C "$source_dir" remote add origin https://github.com/microsoft/onnxruntime.git
    git -C "$source_dir" fetch --depth=1 origin "$revision"
    GIT_LFS_SKIP_SMUDGE=1 git -C "$source_dir" checkout --detach FETCH_HEAD
fi
if [[ ! -d "$source_dir/.git" ]] || [[ "$(git -C "$source_dir" rev-parse HEAD)" != "$revision" ]]; then
    echo "Refusing to use a source directory that is not the pinned ONNX checkout: $source_dir" >&2
    exit 1
fi

status="$(git -C "$source_dir" status --porcelain --untracked-files=normal)"
if [[ -z "$status" ]]; then
    git -C "$source_dir" apply "$patch_file"
elif [[ "$status" != " M $changed_file" ]]; then
    echo 'The ONNX checkout contains unexpected changes; leaving it untouched.' >&2
    exit 1
fi

# Check the whole file, so an unrelated change cannot be hidden by the expected patch.
python3 - "$source_dir" "$revision" "$changed_file" <<'PY'
import pathlib, subprocess, sys
source, revision, name = sys.argv[1:]
original = subprocess.check_output(['git', '-C', source, 'show', revision + ':' + name])
before = b'std::make_unique_for_overwrite<int8_t[]>(lut_buffer_size)'
after = b'std::make_unique<int8_t[]>(lut_buffer_size)'
if original.count(before) != 1 or (pathlib.Path(source) / name).read_bytes() != original.replace(before, after):
    raise SystemExit('The ONNX compatibility patch does not match the expected source.')
PY

"$source_dir/build.sh" --build_dir "$work/build" --config Release \
    --build_shared_lib --skip_tests --skip_submodule_sync --no_telemetry \
    --parallel "$jobs" --cmake_generator Ninja --cmake_extra_defines \
    CMAKE_OSX_DEPLOYMENT_TARGET=14.0 CMAKE_OSX_ARCHITECTURES=arm64 \
    CMAKE_POLICY_VERSION_MINIMUM=3.5 onnxruntime_BUILD_UNIT_TESTS=OFF

python3 - "$work" "$revision" "$patch_file" <<'PY'
import hashlib, json, pathlib, subprocess, sys
work, revision, patch = pathlib.Path(sys.argv[1]), sys.argv[2], pathlib.Path(sys.argv[3])
deps = work / 'build/Release/_deps'
cache = {}
for line in (work / 'build/Release/CMakeCache.txt').read_text().splitlines():
    if ':' in line and '=' in line and not line.startswith(('#', '//')):
        key, value = line.split('=', 1)
        cache[key.split(':', 1)[0]] = value
required = {'CMAKE_OSX_ARCHITECTURES': 'arm64', 'CMAKE_OSX_DEPLOYMENT_TARGET': '14.0',
            'onnxruntime_USE_TELEMETRY': 'OFF', 'onnxruntime_BUILD_UNIT_TESTS': 'OFF'}
for key, value in required.items():
    if cache.get(key) != value:
        raise SystemExit(f'Unexpected ONNX build setting {key}: {cache.get(key)}')
known = {}
for line in (work / 'source/cmake/deps.txt').read_text().splitlines():
    parts = line.split(';')
    if len(parts) == 3:
        known[parts[0]] = (parts[1], parts[2])
aliases = {'eigen3': 'eigen', 'gsl': 'microsoft_gsl', 'nlohmann_json': 'json',
           'protoc_binary': 'protoc_mac_universal'}
archives = []
for subbuild in sorted(deps.glob('*-subbuild')):
    name = subbuild.name[:-len('-subbuild')]
    for path in subbuild.glob('*-populate-prefix/src/*'):
        if not path.is_file() or not path.name.endswith(('.zip', '.tar.gz', '.tgz')):
            continue
        key = aliases.get(name, name)
        if key not in known:
            raise SystemExit(f'No upstream dependency record for {name}')
        url, expected = known[key]
        data = path.read_bytes()
        if hashlib.sha1(data).hexdigest() != expected:
            raise SystemExit(f'Upstream SHA-1 mismatch for {name}')
        archives.append({'dependency': name, 'file': str(path.relative_to(work)), 'url': url,
                         'upstream_sha1': expected, 'sha256': hashlib.sha256(data).hexdigest(),
                         'role': 'prebuilt build tool' if name == 'protoc_binary' else 'source archive'})
if not archives:
    raise SystemExit('No cached dependency archives found; input provenance was not recorded.')
runtime = work / 'build/Release/libonnxruntime.1.30.0.dylib'
compiler = cache['CMAKE_CXX_COMPILER']
version = lambda command: subprocess.check_output(command, text=True).splitlines()[0]
manifest = {'scope': 'Experimental macOS build; downloaded archives, not an exhaustive notice inventory',
            'onnx_commit': revision, 'patch_sha256': hashlib.sha256(patch.read_bytes()).hexdigest(),
            'options': ['Release', 'shared', 'macOS arm64', 'macOS 14 minimum',
                        'POSIX telemetry disabled', 'unit tests disabled'],
            'kleidiai': cache.get('onnxruntime_USE_KLEIDIAI', 'unknown'),
            'toolchain': {'compiler': version([compiler, '--version']),
                          'cmake': version(['cmake', '--version']),
                          'ninja': version(['ninja', '--version']),
                          'apple_sdk': version(['xcrun', '--show-sdk-version'])},
            'runtime_sha256': hashlib.sha256(runtime.read_bytes()).hexdigest(), 'archives': archives}
(work / 'input-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
PY

echo "Experimental runtime: $work/build/Release/libonnxruntime.1.30.0.dylib"
echo "Downloaded-input manifest: $work/input-manifest.json"
echo 'Review its dependencies, notices and tests before using it in a release.'
