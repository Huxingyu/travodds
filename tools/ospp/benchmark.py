#!/usr/bin/env python3
"""Bounded same-host, independent-process DDS performance comparison."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import signal
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--rounds', type=int, default=3)
    args = parser.parse_args()
    if not 1 <= args.rounds <= 10:
        parser.error('rounds must be 1..10')
    binary = args.binary.resolve(strict=True)
    args.output.mkdir(parents=True, exist_ok=False)
    summary = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'passed': False,
               'platform': platform.platform(), 'libc': platform.libc_ver(), 'cpu_count': os.cpu_count(),
               'binary': str(binary), 'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
               'meminfo': Path('/proc/meminfo').read_text(),
               'method': 'Same-host independent Writer/Reader processes, RELIABLE KEEP_LAST16, 64 native slots, batches of8 with receiver acknowledgements; 32 warmup samples excluded from timing, every payload word validated. Throughput includes fill/validation/control/scheduling. Latency from monotonic timestamp immediately before write to take before integrity scan; nearest-rank quantiles. Shared VM with swap, no dedicated-host SLA or maximum sustainable-throughput claim.',
               'results': []}
    for round_number in range(args.rounds):
        modes = ['rtps', 'ordinary', 'loan']
        modes = modes[round_number % 3:] + modes[:round_number % 3]
        for size in [64, 4096, 65536]:
            for mode in modes:
                count = 128 if size == 65536 else 256
                label = f'round-{round_number+1:02d}-{size}-{mode}'
                process = None
                result = {'label': label, 'mode': mode, 'bytes': size, 'samples': count, 'passed': False}
                try:
                    with (args.output/(label+'.log')).open('w') as log:
                        process = subprocess.Popen([str(binary), mode, str(size), str(count)],
                                                   stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
                        try:
                            result['exit_code'] = process.wait(timeout=60)
                        except subprocess.TimeoutExpired:
                            os.killpg(process.pid, signal.SIGKILL); process.wait()
                            raise RuntimeError('Benchmark timed out; not a throughput result')
                    assert result['exit_code'] == 0, result
                    records = [json.loads(s.removeprefix('BENCH_RESULT '))
                               for s in (args.output/(label+'.log')).read_text().splitlines()
                               if s.startswith('BENCH_RESULT ')]
                    assert len(records) == 2
                    roles = {r['role']: r for r in records}
                    assert set(roles) == {'writer', 'reader'}
                    assert roles['writer']['pid'] == process.pid
                    assert roles['reader']['pid'] != process.pid
                    for r in records:
                        assert r['mode'] == mode and r['bytes'] == size and r['samples'] == count
                        assert r['warmup'] == 32 and r['batch'] == 8 and r['peak_rss_kib'] > 0
                    expected = count + 32 if mode == 'rtps' else 0
                    assert roles['writer']['serializations'] == expected
                    assert roles['reader']['deserializations'] == expected
                    assert roles['writer']['seconds'] > 0
                    assert 0 <= roles['reader']['latency_p50_us'] <= roles['reader']['latency_p95_us'] <= roles['reader']['latency_p99_us']
                    directory = Path(roles['writer']['directory'])
                    assert directory.parent == Path('/dev/shm') and directory.name.startswith('travodds-bench-')
                    assert not directory.exists()
                    try: os.killpg(process.pid, 0)
                    except ProcessLookupError: pass
                    else: raise RuntimeError('Benchmark left a process alive')
                    result.update(passed=True, records=roles)
                except Exception as error:
                    result['error'] = str(error)
                    if process is not None:
                        try: os.killpg(process.pid, signal.SIGKILL)
                        except ProcessLookupError: pass
                        process.wait()
                summary['results'].append(result)
                (args.output/'summary.json').write_text(json.dumps(summary, indent=2)+'\n')
                print(label, 'PASS' if result['passed'] else result.get('error'), flush=True)
                if not result['passed']:
                    raise SystemExit(1)
    summary['passed'] = True
    (args.output/'summary.json').write_text(json.dumps(summary, indent=2)+'\n')

if __name__ == '__main__':
    main()
