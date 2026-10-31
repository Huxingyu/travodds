#!/usr/bin/env python3
"""Package small, unchanged verification records; verify them after relocation.

The bundle deliberately omits libraries, executables and build/coverage caches.
Recorded absolute paths remain in raw records, but are never used for verification.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import subprocess
import tarfile
import tempfile


FORMAT = 'travodds-small-evidence-v1'
SUFFIXES = {'.json', '.jsonl', '.xml', '.log', '.ddslog', '.txt', '.md', '.py', '.cpp', '.csv'}
SKIP_DIRS = {'.git', '_deps', 'CMakeFiles', 'install', '__pycache__', 'coverage-cache'}


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, sort_keys=True, indent=2) + '\n')


def safe_relative(name):
    path = PurePosixPath(name)
    if not name or path.is_absolute() or '..' in path.parts or '\\' in name:
        raise ValueError('Unsafe relative path: ' + str(name))
    return path


def inside(root, name):
    relative = safe_relative(name)
    path = root.joinpath(*relative.parts)
    if not path.resolve().is_relative_to(root.resolve()):
        raise ValueError('Path escapes its root: ' + str(name))
    return path


def identity_for(mapping):
    if not isinstance(mapping, dict) or not mapping:
        raise ValueError('Empty or invalid source_sha256 map')
    for name, digest in mapping.items():
        safe_relative(name)
        if not isinstance(digest, str) or not re.fullmatch('[0-9a-f]{64}', digest):
            raise ValueError('Invalid source hash for ' + name)
    encoded = json.dumps(mapping, sort_keys=True, separators=(',', ':')).encode()
    return hashlib.sha256(encoded).hexdigest()


def source_status(source, mapping):
    missing, changed = [], []
    for name, expected in sorted(mapping.items()):
        path = inside(source, name)
        if not path.is_file():
            missing.append(name)
        elif sha(path) != expected:
            changed.append(name)
    return {'matches': not missing and not changed, 'missing': missing, 'changed': changed}


def git_head(source):
    result = subprocess.run(['git', '-C', str(source), 'rev-parse', 'HEAD'],
                            capture_output=True, text=True)
    return result.stdout.strip() if result.returncode == 0 else None


def candidates(root):
    """Do not descend into build trees, even when they contain small text files."""
    for current, directories, filenames in os.walk(root, followlinks=False):
        directories[:] = sorted(name for name in directories
                                if name not in SKIP_DIRS and name != 'build'
                                and not name.startswith('build-')
                                and not (Path(current) / name).is_symlink())
        for name in sorted(filenames):
            yield Path(current) / name


def make_readme(manifest):
    lines = [
        '# TravoDDS portable evidence', '',
        'This bundle contains unchanged small records and frozen source hash maps.',
        'It contains no production libraries, executables, object files or gcov caches.',
        'A verified bundle means its records and source identities agree; it does not rerun tests.', '',
        'Place the matching source checkout and this extracted directory anywhere, then run:', '',
        '```bash',
        'python3 evidence/verify_evidence.py verify --bundle evidence --source source',
        '```', '',
        'Python 3.9 or newer is required. The bundled verifier is itself listed and hashed.',
        'Use `--require-source-match` when the checkout must match every recorded run.',
        'Historical runs can have different identities; mismatches are reported separately.',
        'Raw JSON/logs retain original paths for provenance. The verifier uses only bundle-relative',
        'paths and source-relative hash maps; it does not open those recorded machine paths.', '',
        '[Manifest](manifest.json) lists every included file, hash, size and excluded record.',
        'Rebuild commands come from the supplied source documentation; binaries must be rebuilt.', '',
        '| Raw run record | Mode / scope | Recorded passed | Source identity |',
        '|---|---|---|---|',
    ]
    for run in manifest['runs']:
        lines.append('| [{path}]({path}) | {mode} / {scope} | {passed} | '
                     '[{short}](source-identities/{identity}.json) |'.format(
                         **run, short=run['identity'][:12]))
    if not manifest['runs']:
        lines.append('| No results.json with source_sha256 was supplied | | | |')
    lines += ['', 'Do not treat an old run or a `passed: false` record as proof of a new source revision.',
              'Archive SHA-256 should be distributed alongside the archive to detect transfer damage.', '']
    return '\n'.join(lines)


def package(args):
    source = args.source.resolve()
    roots = [path.resolve() for path in args.evidence]
    output = args.output.resolve()
    if not source.is_dir() or not roots or any(not root.is_dir() for root in roots):
        raise ValueError('Source and evidence roots must be existing directories')
    if len(set(roots)) != len(roots):
        raise ValueError('Evidence roots must be distinct')
    if output.exists() or any(output.is_relative_to(root) for root in roots):
        raise ValueError('Use a new output directory outside all evidence roots')
    archive = args.archive.resolve() if args.archive else None
    if archive and (archive.exists() or archive.is_relative_to(output)):
        raise ValueError('Use a new archive path outside the output directory')
    output.parent.mkdir(parents=True, exist_ok=True)
    manifest = {'format': FORMAT, 'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                'source_git_head': git_head(source), 'files': {}, 'source_identities': {},
                'runs': [], 'excluded': [], 'limits': {
                    'max_file_bytes': args.max_file_mib * 1024 * 1024,
                    'max_total_bytes': args.max_total_mib * 1024 * 1024},
                'omitted': 'build directories, libraries, executables, object files and coverage caches'}
    total = 0
    with tempfile.TemporaryDirectory(prefix='.evidence-stage-', dir=output.parent) as temporary:
        stage = Path(temporary) / 'bundle'
        stage.mkdir()
        for index, root in enumerate(roots, 1):
            label = str(index).zfill(2) + '-' + re.sub('[^A-Za-z0-9_.-]', '_', root.name)
            for original in candidates(root):
                name = (Path('records') / label / original.relative_to(root)).as_posix()
                reason = None
                if original.is_symlink():
                    reason = 'symlink'
                elif original.suffix.lower() not in SUFFIXES:
                    reason = 'not a small evidence file type'
                elif original.stat().st_size > manifest['limits']['max_file_bytes']:
                    reason = 'exceeds per-file limit'
                if reason:
                    manifest['excluded'].append({'path': name, 'reason': reason})
                    continue
                size = original.stat().st_size
                total += size
                if total > manifest['limits']['max_total_bytes']:
                    raise ValueError('Small evidence exceeds total size limit')
                before = sha(original)
                target = inside(stage, name)
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(original, target)
                if sha(target) != before or sha(original) != before:
                    raise ValueError('Evidence changed while copying: ' + name)
                manifest['files'][name] = {'sha256': before, 'bytes': size}
                if original.name == 'results.json':
                    record = json.loads(target.read_text())
                    if 'source_sha256' not in record:
                        continue
                    mapping = record['source_sha256']
                    identity = identity_for(mapping)
                    status = source_status(source, mapping)
                    if args.require_source_match and not status['matches']:
                        raise ValueError('Source does not match recorded run: ' + name)
                    manifest['source_identities'][identity] = {
                        'path': 'source-identities/' + identity + '.json',
                        'input_count': len(mapping), 'packaging_source': status}
                    write_json(stage / 'source-identities' / (identity + '.json'), mapping)
                    manifest['runs'].append({'path': name, 'identity': identity,
                                             'mode': record.get('mode'), 'scope': record.get('scope'),
                                             'passed': record.get('passed')})
        if not manifest['runs']:
            raise ValueError('Supply at least one results.json with a source_sha256 map')
        for path in sorted((stage / 'source-identities').glob('*.json')):
            manifest['files'][path.relative_to(stage).as_posix()] = {
                'sha256': sha(path), 'bytes': path.stat().st_size}
        verifier = stage / 'verify_evidence.py'
        shutil.copyfile(Path(__file__).resolve(), verifier)
        manifest['files']['verify_evidence.py'] = {'sha256': sha(verifier),
                                                  'bytes': verifier.stat().st_size}
        (stage / 'README.md').write_text(make_readme(manifest))
        manifest['files']['README.md'] = {'sha256': sha(stage / 'README.md'),
                                        'bytes': (stage / 'README.md').stat().st_size}
        write_json(stage / 'manifest.json', manifest)
        sizes = [path.stat().st_size for path in stage.rglob('*') if path.is_file()]
        if max(sizes) > manifest['limits']['max_file_bytes']:
            raise ValueError('Generated evidence metadata exceeds per-file size limit')
        bundle_bytes = sum(sizes)
        if bundle_bytes > manifest['limits']['max_total_bytes']:
            raise ValueError('Evidence including generated metadata exceeds total size limit')
        verify_bundle(stage, source, args.require_source_match)
        stage.rename(output)
    result = {'bundle': str(output), 'manifest_sha256': sha(output / 'manifest.json'),
              'files': len(manifest['files']), 'raw_bytes': total,
              'bundle_bytes': bundle_bytes,
              'source_identities': len(manifest['source_identities'])}
    if archive:
        archive.parent.mkdir(parents=True, exist_ok=True)
        with archive.open('xb') as stream:
            with tarfile.open(fileobj=stream, mode='w:gz') as tar:
                tar.add(output, arcname='evidence', recursive=True)
        result['archive'] = str(archive)
        result['archive_sha256'] = sha(archive)
    print(json.dumps(result, ensure_ascii=False, sort_keys=True))


def verify_bundle(bundle, source, require_source_match=False):
    manifest = json.loads((bundle / 'manifest.json').read_text())
    if manifest.get('format') != FORMAT:
        raise ValueError('Unknown bundle format')
    expected = set(manifest['files']) | {'manifest.json'}
    actual = {path.relative_to(bundle).as_posix() for path in bundle.rglob('*') if path.is_file()}
    if expected != actual:
        raise ValueError('Bundle file inventory differs from manifest')
    for name, expected_file in manifest['files'].items():
        path = inside(bundle, name)
        if path.is_symlink() or path.stat().st_size != expected_file['bytes'] or sha(path) != expected_file['sha256']:
            raise ValueError('Evidence hash/size mismatch: ' + name)
    statuses = {}
    for identity, information in manifest['source_identities'].items():
        mapping = json.loads(inside(bundle, information['path']).read_text())
        if identity_for(mapping) != identity or len(mapping) != information['input_count']:
            raise ValueError('Frozen source identity mismatch: ' + identity)
        statuses[identity] = source_status(source, mapping) if source else {'matches': None}
    for run in manifest['runs']:
        record = json.loads(inside(bundle, run['path']).read_text())
        if identity_for(record['source_sha256']) != run['identity'] or run['identity'] not in statuses:
            raise ValueError('Run is not bound to its frozen source identity: ' + run['path'])
        if any(record.get(key) != run[key] for key in ['mode', 'scope', 'passed']):
            raise ValueError('Run metadata differs from raw record: ' + run['path'])
    if require_source_match and (not source or not all(status['matches'] for status in statuses.values())):
        raise ValueError('Source checkout does not match every frozen run')
    return {'bundle_records_verified': True, 'tests_rerun': False,
            'files': len(manifest['files']), 'source_identities': statuses}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    pack = commands.add_parser('package', help='copy small records without rewriting their contents')
    pack.add_argument('--source', type=Path, required=True)
    pack.add_argument('--evidence', type=Path, action='append', required=True)
    pack.add_argument('--output', type=Path, required=True)
    pack.add_argument('--archive', type=Path)
    pack.add_argument('--max-file-mib', type=int, default=4)
    pack.add_argument('--max-total-mib', type=int, default=64)
    pack.add_argument('--require-source-match', action='store_true')
    check = commands.add_parser('verify', help='verify a bundle at any location, optionally with source')
    check.add_argument('--bundle', type=Path, required=True)
    check.add_argument('--source', type=Path)
    check.add_argument('--require-source-match', action='store_true')
    args = parser.parse_args()
    try:
        if args.command == 'package':
            if args.max_file_mib <= 0 or args.max_total_mib <= 0:
                raise ValueError('Size limits must be positive')
            package(args)
        else:
            print(json.dumps(verify_bundle(args.bundle.resolve(),
                args.source.resolve() if args.source else None, args.require_source_match),
                ensure_ascii=False, sort_keys=True))
    except (ValueError, OSError, json.JSONDecodeError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
