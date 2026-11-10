#!/usr/bin/env python3
"""Builds DawIt-<version>-dependencies.json: every third-party component of a release, with its version, source
URL and hashes, licence, notices, DawIt's changes to it, where it ends up on each platform, and how each package
was built.

It joins three things, so the manifest can't drift from what was released:
- tools/dependencies.json, as committed in the complete source archive: what each component is and where it goes;
- third_party/MANIFEST.json in that archive: the URL and hashes of every upstream file the release is built from;
- the build-info-<platform>.json files the release jobs write (tools/record-build-info.py): runner, compiler,
  CMake and SDK.
It also reads the run-time downloads (models, the SoundFont) with their pinned hashes from the app's source.

It stops when an upstream file has no entry in tools/dependencies.json, an entry names a file the archive doesn't
have, or a notice file is missing: fix tools/dependencies.json, not the manifest.

Usage: tools/make-dependency-manifest.py ARCHIVE [--build-info FILE ...] [--out DIR]
"""

import argparse
import json
import pathlib
import re
import sys
import tarfile

PLATFORMS = ('macos-arm64', 'windows-x64', 'linux-x64')


def app_downloads(read, entries):
    """Each models::Model { "file", "url" "url...", "sha256", size } in the named source files."""
    found = []
    pattern = re.compile(r'Model\s+\w*\s*\{\s*(\w+|"[^"]+")\s*,\s*((?:"[^"]*"\s*)+),\s*"([0-9a-f]{64})"\s*,\s*(\d+)')
    for entry in entries:
        text = read(entry['file'])
        matches = pattern.findall(text)
        if not matches:
            raise SystemExit(f'No pinned download found in {entry["file"]}')
        for name, url_parts, sha256, size in matches:
            url = ''.join(re.findall(r'"([^"]*)"', url_parts))
            found.append({'name': entry['name'], 'file': url.rsplit('/', 1)[-1], 'url': url, 'sha256': sha256,
                          'size': int(size), 'licence': entry['licence'], 'notices': entry['notices'],
                          'pinned_in': entry['file']})
    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('archive', help='DawIt-<version>-complete-source.tar.gz')
    parser.add_argument('--build-info', nargs='*', default=[], help='build-info-<platform>.json files')
    parser.add_argument('--out', default='.', help='where to write the manifest (default: here)')
    args = parser.parse_args()

    with tarfile.open(args.archive) as archive:
        names = archive.getnames()
        prefix = names[0].split('/', 1)[0]
        members = {n[len(prefix) + 1:]: n for n in names if n.startswith(prefix + '/')}

        def read(path):
            if path not in members:
                raise SystemExit(f'The archive has no {path}')
            return archive.extractfile(members[path]).read().decode('utf-8')

        mirror = json.loads(read('third_party/MANIFEST.json'))
        catalogue = json.loads(read('tools/dependencies.json'))
        downloads = app_downloads(read, catalogue['app_downloads'])

    files = {f['url']: f for f in mirror['files']}
    used = set()
    problems = []
    components = []
    for entry in catalogue['components']:
        placement = entry['platforms']
        if set(placement) != set(PLATFORMS) or not set(placement.values()) <= set(catalogue['placement']):
            problems.append(f'{entry["name"]}: give one of {sorted(catalogue["placement"])} for each of {PLATFORMS}')
        sources = []
        for url in entry.get('sources', []):
            if url not in files:
                problems.append(f'{entry["name"]}: {url} is not in the complete source archive')
                continue
            used.add(url)
            f = files[url]
            sources.append({'url': url, 'in_archive': f['path'], 'sha256': f['sha256'],
                            **({'sha1': f['sha1']} if 'sha1' in f else {})})
        if not sources and not entry.get('part_of') and not entry.get('downloaded_by'):
            problems.append(f'{entry["name"]}: no source, part_of or downloaded_by')
        for notice in entry['notices']:
            if notice not in members:
                problems.append(f'{entry["name"]}: notice {notice} is not in the archive')
        for change in entry.get('changes', []):
            path = change.split(' ', 1)[0]
            if path not in members:
                problems.append(f'{entry["name"]}: change {path} is not in the archive')
        components.append({k: v for k, v in {
            'name': entry['name'], 'version': entry['version'], 'licence': entry['licence'],
            'notices': entry['notices'], 'changes': entry.get('changes', []), 'platforms': placement,
            'source': sources or None, 'part_of': entry.get('part_of'), 'downloaded_by': entry.get('downloaded_by'),
            'note': entry.get('note')}.items() if v is not None})
    for url in sorted(set(files) - used):
        problems.append(f'{url} ({files[url]["role"]}) has no entry in tools/dependencies.json')
    for download in downloads:
        for notice in download['notices']:
            if notice not in members:
                problems.append(f'{download["name"]}: notice {notice} is not in the archive')
    if problems:
        raise SystemExit('tools/dependencies.json is out of step with this release:\n  ' + '\n  '.join(problems))

    builds = {}
    for path in args.build_info:
        info = json.loads(pathlib.Path(path).read_text(encoding='utf-8'))
        builds[info['platform']] = info
    missing = [p for p in PLATFORMS if p not in builds] if args.build_info else []
    if missing:
        raise SystemExit(f'No build record for {", ".join(missing)}')

    manifest = {
        'dawit_version': mirror['dawit_version'],
        'dawit_commit': mirror['dawit_commit'],
        'about': 'Every third-party component of this release. "source" points into the complete source archive '
                 f'({pathlib.Path(args.archive).name}); "platforms" says where each ends up: ' +
                 '; '.join(f'{k}: {v}' for k, v in catalogue['placement'].items()) + '.',
        'components': components,
        'app_downloads': downloads,
        'system_requirements': {k: v for k, v in catalogue['system_requirements'].items() if k != 'about'},
        'builds': builds,
    }
    out = pathlib.Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    target = out / f'DawIt-{mirror["dawit_version"]}-dependencies.json'
    target.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print(f'{target}: {len(components)} components, {len(downloads)} app downloads, '
          f'{len(builds)} build records')


if __name__ == '__main__':
    sys.exit(main())
