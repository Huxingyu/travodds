#!/usr/bin/env python3
"""Merge pristine two-environment counters and invoke the unchanged collector."""
from pathlib import Path
import datetime
import hashlib
import json
import os
import shutil
import subprocess

ROOT = Path(__file__).resolve().parent.parent
SOURCE = Path('/home/ubuntu/project/travodds/.worktrees/datasharing-progress')
BASELINE = '489067108405b27afbd369245542f5e88d457f18'

def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for chunk in iter(lambda: f.read(1048576), b''):
            h.update(chunk)
    return h.hexdigest()

def inventory(folder, pattern):
    return {str(p): sha(p) for p in sorted(folder.rglob(pattern))}

def main():
    out = ROOT / 'coverage'
    assert not out.exists(), 'Never overwrite a collection attempt'
    assert json.loads((ROOT / 'remote/coverage/results.json').read_text())['modal_phase_passed']
    local_result = json.loads((ROOT / 'local/coverage/results.json').read_text())
    assert local_result['local_phase_passed'] and local_result['combined_mode_passed']
    out.mkdir()
    manifest_path = ROOT / 'source-manifest.json'
    manifest = json.loads(manifest_path.read_text())
    expected_sources = manifest['source_sha256']
    actual_sources = {name: sha(SOURCE / name) for name in expected_sources}
    assert actual_sources == expected_sources
    record = {'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'helper': str(Path(__file__).resolve()), 'helper_sha256': sha(__file__),
              'source_manifest_sha256': sha(manifest_path),
              'implementation_commit': manifest['implementation_commit'],
              'canonical_source_identity_sha256': manifest['canonical_source_identity_sha256'],
              'source_before_sha256': manifest['canonical_source_identity_sha256'],
              'baseline': BASELINE, 'passed': False, 'commands': []}
    def run(label, cmd, timeout):
        with (out / (label + '.log')).open('xb') as log:
            result = subprocess.run(['nice', '-n', '19', *map(str, cmd)],
                cwd=SOURCE, stdout=log, stderr=subprocess.STDOUT, timeout=timeout)
        record['commands'].append({'label': label, 'argv': list(map(str, cmd)),
                                    'exit_code': result.returncode, 'log_sha256': sha(out / (label + '.log'))})
        assert result.returncode == 0, label
    modal, local = ROOT / 'counters/modal', ROOT / 'counters/local'
    before = {str(p): inventory(p, '*.gcda') for p in (modal, local)}
    assert all(before.values())
    record['raw_counter_sha256'] = before
    try:
        run('gcov-tool-version', ['gcov-tool', '--version'], 15)
        merged = out / 'merged-counters'
        run('merge', ['gcov-tool', 'merge', modal, local, '-o', merged], 120)
        notes_root = out / 'notes'
        build = ROOT / 'build/coverage'
        notes = sorted(build.rglob('*.gcno'))
        assert notes
        provenance = json.loads((ROOT / 'remote/coverage/build-provenance.json').read_text())
        expected = {item['path']: item['sha256'] for item in provenance['notes']}
        assert inventory(build, '*.gcno') == expected
        mirrored = []
        for path in notes:
            destination = notes_root / path.relative_to(build)
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, destination)
            counter = merged / str(path.with_suffix('.gcda')).lstrip('/')
            if counter.exists():
                shutil.copyfile(counter, destination.with_suffix('.gcda'))
            mirrored.append({'original': str(path), 'copy': str(destination),
                'sha256': sha(path), 'counter': str(counter) if counter.exists() else None,
                'counter_sha256': sha(counter) if counter.exists() else None,
                'official_excluded': '_deps' in path.parts})
        record['notes'] = mirrored
        record['notes_count'] = len(mirrored)
        record['official_notes_count'] = sum(not row['official_excluded'] for row in mirrored)
        collector = SOURCE / 'tools/ospp/coverage.py'
        record['official_collector_sha256'] = sha(collector)
        run('official-collector', ['python3', collector, '--source', SOURCE,
            '--build-root', notes_root, '--output', out / 'report',
            '--baseline', BASELINE, '--minimum', '70'], 300)
        record['summary'] = json.loads((out / 'report/summary.json').read_text())
        assert record['summary']['translation_units'] == record['official_notes_count']
        assert record['summary']['passed'] and record['summary']['percent'] >= 70
        assert {str(p): inventory(p, '*.gcda') for p in (modal, local)} == before
        assert {name: sha(SOURCE / name) for name in expected_sources} == expected_sources
        assert sha(manifest_path) == record['source_manifest_sha256']
        assert sha(__file__) == record['helper_sha256']
        record['source_after_sha256'] = manifest['canonical_source_identity_sha256']
        record['raw_counters_unchanged'] = True
        record['passed'] = True
    finally:
        record['finished_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        (out / 'results.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps({key: value for key, value in record.items() if key not in ('raw_counter_sha256', 'notes', 'summary')}, indent=2))

if __name__ == '__main__':
    main()
