#!/usr/bin/env python3
"""Freeze committed, complete build inputs; never include credentials/artifacts."""
from pathlib import Path
import gzip
import hashlib
import io
import json
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parent.parent
SOURCE = Path('/home/ubuntu/project/travodds/.worktrees/datasharing-progress')
PATHS = ['src', 'include', 'cmake', 'thirdparty', 'test/unit_payloadpool', 'test/unit_inside',
         'test/googletest-1.17.0.zip', 'example/cpp/data_sharing',
         'example/cpp/hello_world', 'tools/ospp', 'CMakeLists.txt', 'LICENSE']

def sha(data):
    return hashlib.sha256(data).hexdigest()

def main():
    archive = ROOT / 'source.tar.gz'
    manifest = ROOT / 'source-manifest.json'
    assert not archive.exists() and not manifest.exists(), 'Never overwrite frozen evidence'
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=SOURCE, text=True).strip()
    assert not subprocess.check_output(['git', 'status', '--porcelain'], cwd=SOURCE).strip()
    raw = subprocess.check_output(['git', 'archive', head, *PATHS], cwd=SOURCE)
    sources = {}
    with tarfile.open(fileobj=io.BytesIO(raw), mode='r:') as handle:
        for member in handle:
            if member.isdir():
                continue
            assert member.isfile() and member.name not in sources
            data = handle.extractfile(member).read()
            assert (SOURCE / member.name).read_bytes() == data
            sources[member.name] = sha(data)
    compressed = gzip.compress(raw, compresslevel=6, mtime=0)
    archive.write_bytes(compressed)
    identity = sha(json.dumps(sources, sort_keys=True, separators=(',', ':'), ensure_ascii=False).encode())
    record = {'schema': 1, 'implementation_commit': head, 'source_sha256': sources,
              'source_archive_sha256': sha(compressed),
              'canonical_source_identity_sha256': identity,
              'input_count': len(sources), 'archive_paths': PATHS,
              'policy': 'git archive of final code commit, including every selected source/build/dependency input; documentation excluded'}
    manifest.write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps({key: value for key, value in record.items() if key != 'source_sha256'}, indent=2))

if __name__ == '__main__':
    main()
