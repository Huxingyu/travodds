#!/usr/bin/env python3
"""Run one serial verification step, preserving its actual exit and resources."""
import argparse
import datetime
import json
import os
from pathlib import Path
import subprocess
import time
from freeze import verify

p = argparse.ArgumentParser()
p.add_argument('--output', type=Path, required=True)
p.add_argument('--label', required=True)
p.add_argument('--timeout', type=int, default=1800)
p.add_argument('--manifest', type=Path, help='Verify frozen inputs before and after execution')
p.add_argument('command', nargs=argparse.REMAINDER)
a = p.parse_args()
command = a.command[1:] if a.command[:1] == ['--'] else a.command
a.output.mkdir(parents=True, exist_ok=True)
log = a.output / (a.label + '.log')
start = datetime.datetime.now().astimezone().isoformat()
manifest_sha = verify(Path.cwd(), a.manifest) if a.manifest else None
before = {n: subprocess.check_output(c, text=True) for n, c in
          [('memory', ['free', '-m']), ('disk', ['df', '-h', '.', '/dev/shm'])]}
t = time.monotonic()
with log.open('w') as f:
    result = subprocess.run(['timeout', '--kill-after=5s', str(a.timeout) + 's', *command],
                            stdout=f, stderr=subprocess.STDOUT)
event = dict(label=a.label, command=command, cwd=os.getcwd(), start=start,
             end=datetime.datetime.now().astimezone().isoformat(),
             seconds=time.monotonic()-t, exit=result.returncode, log=str(log), resources=before)
event['manifest_sha256'] = manifest_sha
if a.manifest:
    try:
        verify(Path.cwd(), a.manifest)
    except RuntimeError as error:
        event['source_error'] = str(error)
        event['exit'] = 125
with (a.output / 'commands.jsonl').open('a') as f:
    f.write(json.dumps(event, ensure_ascii=False) + '\n')
print(json.dumps(event, ensure_ascii=False), flush=True)
print('\n'.join(log.read_text(errors='replace').splitlines()[-20:]))
raise SystemExit(event['exit'] if event['exit'] >= 0 else 1)
