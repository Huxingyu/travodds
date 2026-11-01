#!/usr/bin/env python3
"""Run the fixed functional matrix once; abort on any failed product result."""
import datetime
import hashlib
import json
import os
from pathlib import Path
import select
import subprocess
import time

ROOT = Path(__file__).resolve().parent
DEADLINE = datetime.datetime(2026, 11, 1, 12, 42, tzinfo=datetime.timezone.utc).timestamp()
NETNS = 'xh20261101'
REMOTE = '/tmp/xhost-20261101'
RESULT = {'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
          'control': None, 'cases': [], 'passed': False, 'case_repeats': 0}
ACTIVE = []
CTL = None


def save():
    (ROOT / 'results.json').write_text(json.dumps(RESULT, indent=2) + '\n')


def request(payload):
    CTL.stdin.write(json.dumps(payload) + '\n')
    CTL.stdin.flush()
    ready, _, _ = select.select([CTL.stdout], [], [], 12)
    if not ready:
        raise RuntimeError('Remote control SSH did not respond')
    line = CTL.stdout.readline()
    if not line:
        raise RuntimeError('Remote control SSH disconnected')
    answer = json.loads(line)
    with (ROOT / 'control-requests.jsonl').open('a') as f:
        f.write(json.dumps({'request': payload, 'response': answer}) + '\n')
    if answer.get('error'):
        raise RuntimeError(str(answer))
    return answer


def local_start(name, args):
    log = (ROOT / (name + '.log')).open('wb')
    cmd = ['sudo', '-n', 'ip', 'netns', 'exec', NETNS, 'taskset', '-c', '0',
           'nice', '-n', '19', str(ROOT / 'xhost'), *map(str, args)]
    proc = subprocess.Popen(cmd, stdout=log, stderr=subprocess.STDOUT,
                            cwd=ROOT, start_new_session=True)
    info = {'host': 'vm', 'name': name, 'pid': proc.pid, 'command': cmd,
            'proc': proc, 'log': log}
    ACTIVE.append(info)
    return info


def remote_start(name, args):
    answer = request({'op': 'start', 'name': name, 'args': args})
    info = {'host': 'remote', 'name': name, 'pid': answer['pid']}
    ACTIVE.append(info)
    return info


def finish(info, limit):
    if info['host'] == 'vm':
        rc = info['proc'].wait(timeout=max(1, limit-time.time()))
        info['log'].close()
        text = (ROOT / (info['name'] + '.log')).read_text(errors='replace')
    else:
        while True:
            answer = request({'op': 'poll', 'pid': info['pid']})
            rc = answer.get('returncode')
            if rc is not None:
                text = answer['log_text']
                (ROOT / (info['name'] + '.log')).write_text(text)
                break
            if time.time() >= limit:
                raise RuntimeError('Remote product process exceeded outer deadline')
            time.sleep(.1)
    info['returncode'] = rc
    if rc != 0:
        raise RuntimeError(f"{info['name']} failed rc={rc}: {text[-2000:]}")
    entries = [json.loads(line.split('XHOST_RESULT ', 1)[1])
               for line in text.splitlines() if line.startswith('XHOST_RESULT ')]
    if len(entries) != 1:
        raise RuntimeError(f"{info['name']}: expected exactly one result")
    return {'host': info['host'], 'pid': info['pid'], 'returncode': rc,
            'natural_exit': True, 'result': entries[0],
            'log': info['name'] + '.log',
            'log_sha256': hashlib.sha256(text.encode()).hexdigest()}


def run_pair(name, kind, mode, size, writer_host, native=False, port=41000):
    if time.time() + 100 >= DEADLINE:
        raise RuntimeError('Time budget reserves cleanup; no new case started')
    expectation = 'native' if native else 'cdr'
    writer_ip = '10.99.0.1' if writer_host == 'vm' else '10.99.0.2'
    writer_args = ['writer', kind, mode, size, 64, writer_ip, port, expectation]
    reader_args = ['reader', kind, mode, size, 64, writer_ip, port, expectation]
    writer = (local_start if writer_host == 'vm' else remote_start)(name+'-writer', writer_args)
    reader = (local_start if native or writer_host != 'vm' else remote_start)(name+'-reader', reader_args)
    limit = min(time.time() + 145, DEADLINE)
    # Both processes must naturally exit, even after emitting their result.
    wr = finish(writer, limit)
    rd = finish(reader, limit)
    w, r = wr['result'], rd['result']
    assert w['role'] == 'writer' and r['role'] == 'reader'
    for result in [w, r]:
        assert result['kind'] == kind and result['mode'] == mode
        assert result['bytes'] == size and result['samples'] == 64
    assert w['serializations'] == (0 if native else 64)
    assert r['deserializations'] == (0 if native else 64)
    assert r['loans'] == (64 if native else 0)
    assert r['validated'] == 64
    assert (w['boot_id'] == r['boot_id']) == native
    record = {'name': name, 'kind': kind, 'mode': mode, 'bytes': size,
              'writer_host': writer_host, 'expectation': expectation,
              'writer': wr, 'reader': rd, 'passed': True}
    print(json.dumps({'case': name, 'passed': True}), flush=True)
    return record


def cleanup_active():
    for info in ACTIVE:
        if info.get('returncode') is not None:
            continue
        try:
            if info['host'] == 'remote':
                request({'op': 'stop', 'pid': info['pid']})
            elif info['proc'].poll() is None:
                # Exact recorded process; never use name matching.
                subprocess.run(['sudo', '-n', 'kill', '-TERM', str(info['pid'])], check=False)
                try:
                    info['proc'].wait(timeout=4)
                except subprocess.TimeoutExpired:
                    subprocess.run(['sudo', '-n', 'kill', '-KILL', str(info['pid'])], check=False)
                    info['proc'].wait(timeout=4)
        except Exception as e:
            RESULT.setdefault('cleanup_errors', []).append(str(e))


if __name__ == '__main__':
    try:
        CTL = subprocess.Popen(['ssh', '-o', 'BatchMode=yes', '-o', 'ConnectTimeout=12',
                                'openlet-eval4090', 'nice -n 19 python3 '+REMOTE+'/remote_ctl.py'],
                               stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=(ROOT/'control-ssh.log').open('w'), text=True,
                               bufsize=1)
        RESULT['control'] = run_pair('control-auto-native', 'auto', 'loan', 64, 'vm', True)
        save()
        n = 0
        for writer_host in ['vm', 'remote']:
            for kind in ['auto', 'on']:
                for mode in ['ordinary', 'loan']:
                    for size in [64, 4096, 65536]:
                        n += 1
                        name = f'{n:02d}-{writer_host}-{kind}-{mode}-{size}'
                        RESULT['cases'].append(run_pair(name, kind, mode, size, writer_host, port=41000+n))
                        save()
        assert len(RESULT['cases']) == 24
        RESULT['passed'] = True
    except Exception as error:
        RESULT['error'] = repr(error)
        print('STOP: ' + repr(error), flush=True)
    finally:
        cleanup_active()
        if CTL is not None:
            CTL.stdin.close()
            try:
                CTL.wait(timeout=5)
            except subprocess.TimeoutExpired:
                CTL.terminate()
                CTL.wait(timeout=5)
        RESULT['finished_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        RESULT['processes'] = [{k:v for k,v in item.items() if k not in ['proc', 'log']}
                               for item in ACTIVE]
        save()
    raise SystemExit(0 if RESULT['passed'] else 1)
