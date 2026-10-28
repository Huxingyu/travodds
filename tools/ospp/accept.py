#!/usr/bin/env python3
"""Serial full/repeated acceptance; validates actual XML test counts and exits."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import shutil
import xml.etree.ElementTree as ET

FILTER = ('LoanIntegration.*AcrossExec*:LoanIntegration.*DestinationPrefix*:'
          'LoanIntegration.ReaderDetach*:ReaderDiscoveryBarrier.*:'
          'LoanIntegration.*Retry*:LoanIntegration.MappingNewSession*:'
          'LoanIntegration.*Autopurge*:LoanIntegration.*Deadline*:'
          'LoanIntegration.Concurrent*:LoanIntegration.TimeBasedFilter*:LoanIntegration.KeepAll*:'
          'LoanIntegration.Lifespan*:NativeReaderIntegration.*FinitePurge*:'
          'TimerSynchronization.*:LoggerSynchronization.*:LoanNotifications.DestroyCondition*')


def run(binary, log, xml, extra=()):
    with log.open('w') as f:
        p = subprocess.run(['timeout', '--kill-after=5s', '650s', str(binary),
                            '--gtest_output=xml:' + str(xml), *extra], stdout=f, stderr=subprocess.STDOUT)
    if p.returncode:
        raise RuntimeError(f'{binary}: exit {p.returncode}, see {log}')
    root = ET.parse(xml).getroot()
    result = dict(root.attrib)
    assert int(result['tests']) > 0 and not int(result.get('failures', 0)) and not int(result.get('errors', 0)), result
    assert not int(result.get('disabled', 0)), result
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--prefix', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--mode', choices=['normal', 'sanitize'], required=True)
    p.add_argument('--stage', choices=['full', 'collect', 'repeat', 'tcp-waitset'], required=True)
    p.add_argument('--rounds', type=int, default=30)
    a = p.parse_args()
    prefix = a.prefix.resolve()
    a.output.mkdir(parents=True, exist_ok=False)
    if a.mode == 'sanitize':
        os.environ['ASAN_OPTIONS'] = 'detect_leaks=0:halt_on_error=1'
        os.environ['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
        core, pools = prefix/'build-core/unit_test', prefix/'build-tests'
    else:
        core, pools = prefix/'build-unit_inside/unit_test', prefix/'build-unit_payloadpool'
    results = {}
    if a.stage == 'collect':
        assert a.mode == 'sanitize', 'collect consumes the complete sanitize.sh run'
        for name in ['core', 'payloadpool', 'sharedpayloadpool']:
            source = prefix/'results'/(name+'.xml')
            result = dict(ET.parse(source).getroot().attrib)
            assert int(result['tests']) > 0 and not int(result['failures']) and not int(result.get('errors', 0))
            shutil.copy2(source, a.output/(name+'.xml'))
            shutil.copy2(prefix/'logs'/('test-'+name+'.log'), a.output/(name+'.log'))
            results[name] = result
    elif a.stage == 'full':
        for name, binary in [('core', core), ('payloadpool', pools/'payloadpool_test'),
                             ('sharedpayloadpool', pools/'sharedpayloadpool_test')]:
            results[name] = run(binary, a.output/(name+'.log'), (a.output/(name+'.xml')).resolve())
            print(name, results[name], flush=True)
    else:
        binary = pools/'payloadpool_test' if a.stage == 'repeat' else core
        selection = FILTER if a.stage == 'repeat' else '*TCP*:*Tcp*:*tcp*:WaitSetTest.*'
        log, xml = a.output/'repeat.log', (a.output/'repeat.xml').resolve()
        results['last_iteration'] = run(binary, log, xml, [f'--gtest_filter={selection}', f'--gtest_repeat={a.rounds}'])
        chunks = re.split(r'Repeating all tests \(iteration (\d+)\)', log.read_text(errors='replace'))
        rounds = chunks[1::2]
        assert rounds == list(map(str, range(1, a.rounds+1))), rounds
        counts = []
        for chunk in chunks[2::2]:
            n = re.search(r'\[==========\] Running (\d+) tests?', chunk)
            assert n and int(n[1]) > 0
            count = int(n[1]); counts.append(count)
            assert f'[  PASSED  ] {count} tests.' in chunk
            assert '[  FAILED  ]' not in chunk
        assert len(set(counts)) == 1
        results.update(rounds=a.rounds, tests_per_round=counts[0], parent_executions=sum(counts), filter=selection)
    (a.output/'summary.json').write_text(json.dumps(dict(passed=True, **results), indent=2)+'\n')
    print(json.dumps(results, indent=2))


if __name__ == '__main__':
    main()
