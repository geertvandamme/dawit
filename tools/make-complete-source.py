#!/usr/bin/env python3
"""Builds DawIt-<version>-complete-source.tar.gz: everything needed to rebuild a DawIt release without a network.

The archive holds DawIt's own files at one commit (as `git archive` gives them) and, in third_party/mirror/, the
unmodified upstream download of every pinned component the build uses: JUCE with the VST3 SDK, demucs.cpp, Eigen,
whisper.cpp with ggml, TinySoundFont, stb_vorbis, the Basic Pitch model, the CMU dictionary, the Bravura font, and
ONNX Runtime's source with the dependencies its build fetches. third_party/MANIFEST.json lists each file with its
URL and hashes. docs/complete-source.md, inside the archive, says how to rebuild from it.

The build reads the mirror by itself: cmake/DawItPinned.cmake for DawIt's own downloads, and the ONNX scripts in
tools/ for ONNX Runtime and its dependencies. Every file is checked against the hash pinned in the source before it
goes in, and the archive is reproducible: the same commit gives the same bytes.

Usage: tools/make-complete-source.py [--commit REV] [--out DIR] [--cache DIR]
"""

import argparse
import gzip
import hashlib
import io
import json
import pathlib
import re
import subprocess
import sys
import tarfile
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parent.parent

# ONNX Runtime dependencies fetched by the release builds (macOS arm64, Linux x64, Windows x64), by their name in
# ONNX's cmake/deps.txt. A missing one makes an offline build stop with a download error, never use other code.
ONNX_DEPENDENCIES = [
    'abseil_cpp', 'date', 'eigen', 'flatbuffers', 'googletest', 'json', 'kleidiai', 'microsoft_gsl',
    'microsoft_wil', 'mp11', 'onnx', 'protobuf', 'pytorch_cpuinfo', 're2', 'safeint',
    'protoc_mac_universal', 'protoc_linux_x64', 'protoc_win64',   # prebuilt protoc, run during the build only
]

# What each of DawIt's pinned downloads is, by its name in CMakeLists.txt (dawit_pinned).
DAWIT_ROLES = {
    'JUCE': 'JUCE 8.0.9 with the VST3 SDK: source',
    'DEMUCS': 'demucs.cpp: source',
    'EIGEN': 'Eigen (for demucs.cpp): source',
    'WHISPER': 'whisper.cpp 1.9.5 with ggml: source',
    'TINYSOUNDFONT': 'TinySoundFont 0.9: source',
    'STBVORBIS': 'stb_vorbis 1.22: source',
    'BASIC_PITCH': 'Basic Pitch model, built into the app',
    'CMUDICT': 'CMU Pronouncing Dictionary, built into the app',
    'BRAVURA_OTF': 'Bravura font, built into the app',
    'BRAVURA_LICENSE': 'Bravura font licence',
}


def git(*args, binary=False):
    output = subprocess.check_output(['git', '-C', str(ROOT), *args])
    return output if binary else output.decode()


def mirror_path(url):
    if not url.startswith('https://'):
        raise SystemExit(f'Not an https URL: {url}')
    return url[len('https://'):]


def pinned_downloads(cmake):
    """Every dawit_pinned(NAME url sha256) call in CMakeLists.txt, with ${VAR} resolved from literal set() calls."""
    variables = dict(re.findall(r'^\s*set\((\w+)\s+"?([^\s")]+)"?\s*\)', cmake, re.M))
    found = []
    for name, url, sha256 in re.findall(r'dawit_pinned\((\w+)\s+(\S+)\s+([0-9a-f]{64})\)', cmake):
        resolved = re.sub(r'\$\{(\w+)\}', lambda m: variables[m.group(1)], url)
        if '$' in resolved:
            raise SystemExit(f'Unresolved variable in {name}: {url}')
        found.append({'name': name, 'url': resolved, 'sha256': sha256})
    calls = len(re.findall(r'^\s*dawit_pinned\(', cmake, re.M))
    if calls != len(found) or not found:
        raise SystemExit(f'Read {len(found)} of {calls} dawit_pinned calls; keep each on one line.')
    unknown = [d['name'] for d in found if d['name'] not in DAWIT_ROLES]
    if unknown:
        raise SystemExit(f'Say what these downloads are in DAWIT_ROLES: {", ".join(unknown)}')
    return found


def onnx_source(commit):
    """The pinned ONNX revision and tarball hash, the same in all three build scripts."""
    pins = set()
    for script in ('tools/build-onnxruntime-source-macos.sh', 'tools/build-onnxruntime-source-linux.sh',
                   'tools/build-onnxruntime-source-windows.ps1'):
        text = git('show', f'{commit}:{script}')
        revision = re.search(r"""^\$?revision\s*=\s*'?([0-9a-f]{40})'?""", text, re.M)
        sha256 = re.search(r"""^\$?(?:source_archive_sha256|sourceArchiveSha256)\s*=\s*'?([0-9a-f]{64})'?""",
                           text, re.M)
        if not revision or not sha256:
            raise SystemExit(f'No ONNX revision or tarball hash in {script}')
        pins.add((revision.group(1), sha256.group(1)))
    if len(pins) != 1:
        raise SystemExit(f'The ONNX build scripts pin different sources: {sorted(pins)}')
    revision, sha256 = pins.pop()
    return {'name': 'ONNX_RUNTIME', 'sha256': sha256, 'revision': revision,
            'url': f'https://github.com/microsoft/onnxruntime/archive/{revision}.tar.gz'}


def fetch(cache, url, algorithm, expected):
    """The file at url, from the cache when it's there, checked against its pinned hash."""
    path = cache / mirror_path(url)
    if not path.is_file():
        path.parent.mkdir(parents=True, exist_ok=True)
        print(f'Downloading {url}', flush=True)
        request = urllib.request.Request(url, headers={'User-Agent': 'DawIt-source-archive'})
        partial = path.with_name(path.name + '.part')
        with urllib.request.urlopen(request, timeout=120) as response, open(partial, 'wb') as out:
            while block := response.read(1 << 20):
                out.write(block)
        partial.replace(path)
    digest = hashlib.new(algorithm)
    with open(path, 'rb') as data:
        while block := data.read(1 << 20):
            digest.update(block)
    if digest.hexdigest() != expected:
        path.unlink()
        raise SystemExit(f'{algorithm} mismatch for {url}: the download was removed from the cache.')
    return path


def onnx_dependencies(cache, onnx_tarball, revision):
    with tarfile.open(onnx_tarball) as archive:
        deps = archive.extractfile(f'onnxruntime-{revision}/cmake/deps.txt').read().decode()
    known = {}
    for line in deps.splitlines():
        parts = line.split(';')
        if len(parts) == 3 and not line.startswith('#'):
            known[parts[0]] = (parts[1], parts[2])
    entries = []
    for name in ONNX_DEPENDENCIES:
        if name not in known:
            raise SystemExit(f'ONNX deps.txt has no {name}')
        url, sha1 = known[name]
        path = fetch(cache, url, 'sha1', sha1)
        role = ('ONNX Runtime build tool (prebuilt protoc; its source is protobuf)' if name.startswith('protoc_')
                else 'ONNX Runtime dependency: source')
        entries.append({'name': name, 'url': url, 'sha1': sha1, 'path': path, 'role': role})
    return entries


def sha256_of(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as data:
        while block := data.read(1 << 20):
            digest.update(block)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('--commit', default='HEAD', help='the DawIt commit or tag to archive (default HEAD)')
    parser.add_argument('--out', default=str(ROOT / 'dist'), help='where to write the archive (default dist/)')
    parser.add_argument('--cache', default=str(ROOT / 'build/complete-source-cache'),
                        help='where downloads are kept between runs')
    args = parser.parse_args()

    commit = git('rev-parse', '--verify', f'{args.commit}^{{commit}}').strip()
    cmake = git('show', f'{commit}:CMakeLists.txt')
    version = re.search(r'^project\(DawIt VERSION ([0-9.]+)', cmake, re.M).group(1)
    timestamp = int(git('show', '-s', '--format=%ct', commit).strip())
    prefix = f'DawIt-{version}-complete-source'
    cache = pathlib.Path(args.cache).resolve()

    entries = []
    for pin in pinned_downloads(cmake):
        path = fetch(cache, pin['url'], 'sha256', pin['sha256'])
        entries.append({**pin, 'path': path, 'role': DAWIT_ROLES[pin['name']]})
    onnx = onnx_source(commit)
    onnx_tarball = fetch(cache, onnx['url'], 'sha256', onnx['sha256'])
    entries.append({**onnx, 'path': onnx_tarball, 'role': f'ONNX Runtime 1.30.0 (commit {onnx["revision"]}): source'})
    entries += onnx_dependencies(cache, onnx_tarball, onnx['revision'])

    manifest = {
        'dawit_version': version,
        'dawit_commit': commit,
        'about': 'Unmodified upstream downloads, kept at their URL without "https://" under third_party/mirror/. '
                 "DawIt's own changes to them are in tools/patches/. Demucs and Whisper models and the MuseScore "
                 'General SoundFont are not here: the app downloads them on first use, after asking.',
        'files': [{'path': f'third_party/mirror/{mirror_path(e["url"])}', 'url': e['url'], 'role': e['role'],
                   'sha256': sha256_of(e['path']), **({'sha1': e['sha1']} if 'sha1' in e else {})}
                  for e in entries],
    }

    out_dir = pathlib.Path(args.out).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    archive_path = out_dir / f'{prefix}.tar.gz'
    partial = archive_path.with_name(archive_path.name + '.part')

    def normalised(info):
        info.mtime, info.uid, info.gid, info.uname, info.gname = timestamp, 0, 0, '', ''
        return info

    with open(partial, 'wb') as raw, gzip.GzipFile(filename='', mode='wb', fileobj=raw, mtime=0) as zipped, \
            tarfile.open(fileobj=zipped, mode='w', format=tarfile.PAX_FORMAT) as out:
        # DawIt's own files, exactly as git has them at the commit.
        own = subprocess.Popen(['git', '-C', str(ROOT), 'archive', '--format=tar', f'--prefix={prefix}/', commit],
                               stdout=subprocess.PIPE)
        with tarfile.open(fileobj=own.stdout, mode='r|') as source:
            for member in source:
                if member.name == 'pax_global_header':
                    continue
                out.addfile(normalised(member), source.extractfile(member) if member.isfile() else None)
        if own.wait() != 0:
            raise SystemExit('git archive failed')

        def add_directory(name):
            info = normalised(tarfile.TarInfo(f'{prefix}/{name}'))
            info.type, info.mode = tarfile.DIRTYPE, 0o755
            out.addfile(info)

        def add_file(name, data):
            info = normalised(tarfile.TarInfo(f'{prefix}/{name}'))
            info.size, info.mode = len(data), 0o644
            out.addfile(info, io.BytesIO(data))

        files = sorted(manifest['files'], key=lambda f: f['path'])
        directories = sorted({str(parent) for f in files for parent in pathlib.PurePosixPath(f['path']).parents
                              if str(parent) != '.'})
        for directory in directories:
            add_directory(directory)
        add_file('third_party/MANIFEST.json', (json.dumps(manifest, indent=2) + '\n').encode())
        paths = {f'third_party/mirror/{mirror_path(e["url"])}': e['path'] for e in entries}
        for f in files:
            info = normalised(out.gettarinfo(str(paths[f['path']]), arcname=f'{prefix}/{f["path"]}'))
            info.mode = 0o644
            with open(paths[f['path']], 'rb') as data:
                out.addfile(info, data)
    partial.replace(archive_path)

    digest = sha256_of(archive_path)
    (out_dir / f'{archive_path.name}.sha256').write_text(f'{digest}  {archive_path.name}\n')
    print(f'{archive_path} ({archive_path.stat().st_size / 1e6:.0f} MB)')
    print(f'SHA-256 {digest}')
    print(f'DawIt {version} at {commit}, with {len(files)} upstream files.')


if __name__ == '__main__':
    sys.exit(main())
