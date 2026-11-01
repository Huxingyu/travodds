#!/usr/bin/env python3
"""Read-only final resource/protected-directory inspection."""
from pathlib import Path
import datetime
import hashlib
import json
import os
import subprocess

ROOT = Path(__file__).resolve().parent.parent
PROTECTED = [ROOT.parent / 'strict-ring-20261031', ROOT.parent / 'portable-evidence-20261031']

def main():
    folder = ROOT / 'audit'
    folder.mkdir(exist_ok=True)
    target = folder / 'cleanup.json'
    apps_path = folder / 'modal-app-list.json'
    assert not target.exists() and not apps_path.exists(), 'Never overwrite cleanup evidence'
    result = subprocess.run(['nice', '-n', '19', '/home/ubuntu/.local/bin/modal', 'app', 'list', '--json'],
        capture_output=True, text=True, timeout=45)
    apps_path.write_text(result.stdout)
    (folder / 'modal-app-list.stderr.log').write_text(result.stderr)
    assert result.returncode == 0
    apps = json.loads(result.stdout)
    ours = [r for r in apps if r.get('description') in ('travodds-final-statistics', 'travodds-final-20261101')]
    assert ours and all(str(r['state']).lower() == 'stopped' and int(r['tasks']) == 0 for r in ours), 'Task app still active'
    expected = json.loads((ROOT / 'protected-before.json').read_text())
    actual = {}
    for root in PROTECTED:
        for path in root.rglob('*'):
            s = path.lstat()
            actual[str(path)] = [s.st_mode, s.st_size, s.st_mtime_ns, s.st_ino]
    before = json.loads((ROOT / 'resource-plan.json').read_text())['shm_before']
    after = sorted(os.listdir('/dev/shm'))
    record = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'shm_before': before, 'shm_after': after, 'shm_added': sorted(set(after) - set(before)),
        'shm_removed': sorted(set(before) - set(after)), 'deleted_local_shm': False,
        'protected': {'passed': actual == expected, 'pathset_equal': set(actual) == set(expected),
            'metadata_equal': actual == expected, 'paths': len(actual),
            'changed': sorted(k for k in set(actual) & set(expected) if actual[k] != expected[k]),
            'added': sorted(set(actual) - set(expected)), 'removed': sorted(set(expected) - set(actual))},
        'modal_apps': ours, 'modal_app_list_sha256': hashlib.sha256(apps_path.read_bytes()).hexdigest()}
    record['passed'] = record['protected']['passed'] and not record['shm_added']
    target.write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record, indent=2))
    assert record['passed']

if __name__ == '__main__':
    main()
