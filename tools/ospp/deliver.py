#!/usr/bin/env python3
"""Audit real acceptance artifacts and package an applicable uncommitted patch."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from freeze import verify, sha


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', type=Path, default=Path(__file__).resolve().parents[2])
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--baseline', default='489067108405b27afbd369245542f5e88d457f18')
    p.add_argument('--normal', type=Path, required=True)
    p.add_argument('--sanitize', type=Path, required=True)
    a = p.parse_args()
    root, out = a.source.resolve(), a.output.resolve()
    frozen = verify(root, out/'frozen-source.json')
    def read(name):
        x = json.loads((out/name).read_text())
        return x
    events = [json.loads(s) for s in (out/'commands.jsonl').read_text().splitlines()]
    final = {e['label']: e for e in events if e['label'].startswith('final-')}
    required = ['final-normal-build', 'final-normal-full', 'final-normal-repeat',
                'final-normal-examples', 'final-sanitize-build-test', 'final-sanitize-full',
                'final-sanitize-repeat', 'final-sanitize-examples', 'final-smoke',
                'final-tcp-waitset', 'final-coverage', 'final-benchmark-build', 'final-benchmark']
    for name in required:
        assert name in final and final[name]['exit'] == 0, (name, final.get(name))
        assert final[name]['manifest_sha256'] == frozen, name
    results = {}
    for mode in ['normal', 'sanitize']:
        for stage in ['full', 'repeat', 'examples']:
            x = read(mode+'-'+stage+'/summary.json')
            assert x['passed'], (mode, stage)
            if stage == 'repeat': assert x['rounds'] == 30
            if stage == 'examples':
                assert x['requested_rounds'] == 30 and len(x['results']) == 90
                assert all(r['passed'] and r['exit_code'] == 0 and not r['stopped_by_runner'] for r in x['results'])
            results[mode+'-'+stage] = x
    for name in ['core', 'payloadpool', 'sharedpayloadpool']:
        assert results['normal-full'][name]['tests'] == results['sanitize-full'][name]['tests']
    before = ET.parse(out/'review-before.xml').getroot()
    assert int(before.attrib['tests']) == 3 and int(before.attrib['failures']) == 3
    regression_names = {
        'KeyedReviewExpiredAdmissionForNativeAndCdrBothReliabilities',
        'ReviewRejectedSequencePreservesSampleAcrossAllVariants',
        'ReviewKeepLastCapacityMustSustainWritesOrRejectAtCreation'}
    regressions = {}
    for mode in ['normal', 'sanitize']:
        cases = [c for c in ET.parse(out/(mode+'-full/payloadpool.xml')).iter('testcase')
                 if c.attrib['name'] in regression_names]
        assert len(cases) == 3 and not any(c.find('failure') is not None for c in cases)
        regressions[mode] = [c.attrib['classname']+'.'+c.attrib['name'] for c in cases]
    smoke = read('smoke/summary.json')
    assert all(x['passed'] for x in smoke['results'])
    coverage = read('coverage/summary.json')
    assert coverage['passed'] and coverage['percent'] >= 70 and coverage['baseline'] == a.baseline
    for name, data in coverage['files'].items(): assert sha(root/name) == data['source_sha256'], name
    benchmark = read('benchmark/summary.json')
    assert benchmark['passed'] and len(benchmark['results']) == 27
    changed = subprocess.check_output(['git','ls-files','--modified','--others','--exclude-standard','-z'], cwd=root).decode().split('\0')
    changed = sorted(n for n in set(changed) if n and (root/n).is_file() and
                     not n.endswith(('.ddslog','.pyc')) and '__pycache__' not in n)
    headers, libraries, links = {}, {}, {}
    for mode, prefix in [('normal', a.normal.resolve()), ('sanitize', a.sanitize.resolve())]:
        for n in changed:
            if not n.startswith('include/'): continue
            same = [f for f in (prefix/'install/include').rglob(Path(n).name) if sha(f) == sha(root/n)]
            assert same, (mode, n)
            headers[mode+':'+n] = str(same[0])
        libs = [f for f in (prefix/'install/lib').glob('libtravodds*.so.*') if not f.is_symlink()]
        assert libs
        libraries[mode] = {f.name: sha(f) for f in libs}
        bins = ['build-unit_inside/unit_test','build-unit_payloadpool/payloadpool_test','build-unit_payloadpool/sharedpayloadpool_test'] if mode == 'normal' else ['build-core/unit_test','build-tests/payloadpool_test','build-tests/sharedpayloadpool_test']
        bins += ['build-data_sharing/datasharing_loan']
        for b in bins:
            text = subprocess.check_output(['ldd', str(prefix/b)]).decode()
            assert str(prefix/'install/lib') in text, text
            if mode == 'sanitize': assert 'libasan.so' in text and 'libubsan.so' in text
            links[mode+':'+b] = dict(sha256=sha(prefix/b), ldd=text)
    subprocess.run(['git','diff','--check'], cwd=root, check=True)
    patch = subprocess.check_output(['git','diff','--binary',a.baseline],cwd=root)
    tracked = set(subprocess.check_output(['git','ls-files','-z'],cwd=root).decode().split('\0'))
    for n in changed:
        if n not in tracked:
            r = subprocess.run(['git','diff','--no-index','--binary','--','/dev/null',n],cwd=root,stdout=subprocess.PIPE)
            assert r.returncode == 1
            patch += r.stdout
    (out/'final.patch').write_bytes(patch)
    with tempfile.TemporaryDirectory(prefix='patch-check-',dir=out) as temp:
        target = Path(temp)
        for n in changed:
            if n in tracked:
                data = subprocess.check_output(['git','show',a.baseline+':'+n],cwd=root)
                (target/n).parent.mkdir(parents=True,exist_ok=True); (target/n).write_bytes(data)
        subprocess.run(['git','apply','--check',str(out/'final.patch')],cwd=target,
                       env={**os.environ,'GIT_CEILING_DIRECTORIES':str(out)},check=True)
    (out/'delivery-source.json').write_text(json.dumps({n:sha(root/n) for n in changed},indent=2)+'\n')
    result = dict(time=datetime.datetime.now().astimezone().isoformat(), baseline=a.baseline,
                  frozen_source_sha256=frozen, delivery_source_sha256=sha(out/'delivery-source.json'),
                  patch_sha256=sha(out/'final.patch'), patch_apply_check=True,
                  commands=events, tests=results, review_regressions=dict(before_tests=3, before_failures=3, after=regressions),
                  checkers=read('checker-environment.json'), smoke=smoke, coverage=coverage,
                  benchmark=benchmark, headers=headers, libraries=libraries, binaries=links,
                  remaining_shared_segments=[str(p) for p in Path('/dev/shm').glob('travodds_ds_*.shm')],
                  limitations=['LeakSanitizer is unavailable under host ptrace; ThreadSanitizer scope and actual outcome are recorded in checkers',
                               'Historical stackless WaitSet SIGSEGV has no proven root cause',
                               'Ubuntu SECURITY=OFF; no Windows, separate openEuler kernel or DDS Security acceptance',
                               'Zero-copy applies to eligible fixed-layout Writer Loan -> Reader Loan payloads only; metadata uses robust locks',
                               'CORRUPT sessions require endpoint/session recreation; BEST_EFFORT has no retransmission guarantee',
                               'Boost 1.88 private file_wrapper is pinned for custom directories; shared VM benchmarks are not an SLA'])
    (out/'verification.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps(dict(verified=True, frozen_source_sha256=frozen, patch_sha256=result['patch_sha256']),indent=2))


if __name__ == '__main__': main()
