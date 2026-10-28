#!/usr/bin/env python3
"""Freeze/check all repository code and build inputs, excluding generated output.

Documentation is packaged separately at delivery, so it can report acceptance
results without changing the identity of the source that produced them.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inputs(root):
    names = subprocess.check_output(
        ['git', 'ls-files', '--cached', '--others', '--exclude-standard', '-z'], cwd=root
    ).decode().split('\0')
    return {n: sha(root / n) for n in sorted(set(names)) if n and
            (root / n).is_file() and not n.startswith(('docs/', '.git/')) and
            not n.endswith(('.ddslog', '.pyc')) and '__pycache__' not in n}


def verify(root, manifest):
    saved = json.loads(manifest.read_text())
    current = inputs(root)
    differences = sorted(n for n in saved.keys() | current.keys() if saved.get(n) != current.get(n))
    if differences:
        raise RuntimeError('Frozen inputs changed: ' + repr(differences))
    return sha(manifest)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', type=Path, default=Path(__file__).resolve().parents[2])
    p.add_argument('--manifest', type=Path, required=True)
    p.add_argument('--check', action='store_true')
    a = p.parse_args()
    if not a.check:
        a.manifest.parent.mkdir(parents=True, exist_ok=True)
        a.manifest.write_text(json.dumps(inputs(a.source), indent=2) + '\n')
    print(verify(a.source, a.manifest))
