#!/usr/bin/env python3
"""Small, serial notification/protocol checks; reuses a verified production build.

This does not rebuild DDS, run its complete suite, or validate a new DDS backend.
The experimental protocol and the production boundary tests remain separate.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import resource
import subprocess
import time
import xml.etree.ElementTree as ET


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--mode', choices=['normal', 'asan', 'tsan'], default='normal')
    parser.add_argument('--phase', choices=['all', 'prototype', 'baseline'], default='all')
    parser.add_argument('--leak-check', choices=['on', 'off'], default='on',
                        help='ASan LeakSanitizer scan; disable only with an independently recorded environment limitation')
    parser.add_argument('--repeat', type=int, default=1)
    args = parser.parse_args()
    if not 1 <= args.repeat <= 100:
        parser.error('--repeat must be between 1 and 100')
    source = Path(__file__).resolve().parents[2]
    baseline = args.baseline_root.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    if (output / 'results.json').exists():
        parser.error('output already has results.json; choose a fresh directory')
    results = {
        'source': str(source), 'baseline': str(baseline), 'mode': args.mode,
        'scope': 'experimental protocol and production shared-pool boundary tests; not whole DDS',
        'production_library_rebuilt': False, 'commands': [], 'source_sha256': {},
    }
    results['harness_sha256'] = digest(Path(__file__))
    (output / 'harness.py').write_bytes(Path(__file__).read_bytes())
    tracked_inputs = [source / 'test/unit_payloadpool/test_sharedpayloadpool.cpp']
    tracked_inputs += sorted((source / 'experimental/lockfree_pool').glob('*.hpp'))
    tracked_inputs += sorted((source / 'experimental/lockfree_pool').glob('*.cpp'))
    tracked_inputs += sorted((source / 'experimental/lockfree_pool').glob('*.py'))
    tracked_inputs += [source / 'experimental/lockfree_pool/CMakeLists.txt']
    results['source_sha256'] = {str(p.relative_to(source)): digest(p) for p in tracked_inputs}
    production_snapshot = {}
    library = None

    def save():
        (output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')

    def compiler_limits():
        # Limit only compiler processes. ASan/TSan runtime requires a large
        # virtual address range, so this must not be inherited by test runs.
        limit = 768 * 1024 * 1024
        resource.setrlimit(resource.RLIMIT_AS, (limit, limit))
        os.nice(10)

    def run(name, command, *, build=False, env=None, seconds=45):
        wrapped = ['timeout', '--kill-after=5s', str(seconds) + 's', '/usr/bin/time', '-v', '-o',
                   str(output / (name + '-resources.txt')), *map(str, command)]
        start = time.monotonic()
        process = subprocess.run(wrapped, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                 text=True, env=env, preexec_fn=compiler_limits if build else None)
        (output / (name + '.log')).write_text(process.stdout)
        entry = {'name': name, 'command': wrapped, 'exit_code': process.returncode,
                 'seconds': time.monotonic() - start}
        results['commands'].append(entry)
        save()
        print(name, 'exit', process.returncode, flush=True)
        if process.returncode:
            print(process.stdout[-5000:], flush=True)
            raise RuntimeError(name + ' failed; original log retained')

    flags = ['-std=c++17', '-O1', '-g0', '-pthread']
    env = os.environ.copy()
    if args.mode == 'asan':
        flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
        env['ASAN_OPTIONS'] = 'detect_leaks=' + ('1' if args.leak_check == 'on' else '0') + ':halt_on_error=1'
        env['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
        # The frozen ASan library also carries gcov instrumentation. Redirect
        # runtime counters so this run cannot overwrite historical build data.
        env['GCOV_PREFIX'] = str(output / 'gcov')
        env['GCOV_PREFIX_STRIP'] = '0'
        results['leak_check'] = 'enabled' if args.leak_check == 'on' else 'not executed; explicitly disabled'
    if args.mode == 'tsan':
        flags += ['-fsanitize=thread', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
        env['TSAN_OPTIONS'] = 'halt_on_error=1:exitcode=66'
    prefix_run = ['setarch', platform.machine(), '-R'] if args.mode == 'tsan' else []

    try:
        if args.phase in ('all', 'prototype'):
            for target, name in [('test_pool.cpp', 'prototype'), ('exec_demo.cpp', 'exec-demo'),
                                 ('test_controller.cpp', 'controller')]:
                executable = output / name
                run('build-' + name, ['g++', *flags, '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                    source / 'experimental/lockfree_pool' / target, '-o', executable], build=True, seconds=90)
                for index in range(args.repeat):
                    run(name + '-' + str(index + 1), [*prefix_run, executable], env=env)
        if args.phase in ('all', 'baseline'):
            if args.mode == 'tsan':
                raise RuntimeError('No TSan production-library build in this harness; use prototype phase')
            old = baseline / 'artifacts' / ('local-loan-sanitize' if args.mode == 'asan' else 'local-loan')
            libname = 'libtravoddsd.so.1.1.0' if args.mode == 'asan' else 'libtravodds.so.1.1.0'
            library = old / 'install/lib' / libname
            manifest = json.loads((baseline / 'artifacts/completion-20261028/verification.json').read_text())
            manifest_mode = 'sanitize' if args.mode == 'asan' else 'normal'
            expected = manifest['libraries'][manifest_mode][libname]
            if digest(library) != expected:
                raise RuntimeError('Production library differs from frozen verification manifest')
            frozen = json.loads((baseline / 'artifacts/completion-20261028/frozen-source.json').read_text())
            production_inputs = {p: h for p, h in frozen.items() if p.startswith(('include/', 'src/'))}
            mismatches = []
            normalized = {}
            for path, expected_hash in production_inputs.items():
                actual = digest(source / path)
                production_snapshot[path] = actual
                if actual == expected_hash:
                    continue
                # The committed feature removed one terminal blank line from
                # this file after the frozen build. Verify that exact byte-only
                # difference, rather than silently accepting arbitrary changes.
                if (path == 'src/common/memory/payloadbuffer.cpp' and
                        hashlib.sha256((source / path).read_bytes() + b'\n').hexdigest() == expected_hash):
                    normalized[path] = {'actual_sha256': actual, 'frozen_sha256': expected_hash,
                                        'difference': 'one terminal blank line removed'}
                else:
                    mismatches.append(path)
            if mismatches:
                raise RuntimeError('Production sources differ from reused frozen library: ' + repr(mismatches))
            results['production_library'] = {'path': str(library), 'sha256': expected,
                'verified_frozen_production_inputs': len(production_inputs),
                'exact_terminal_newline_differences': normalized}
            tests = old / ('build-tests' if args.mode == 'asan' else 'build-unit_payloadpool')
            executable = output / 'production-boundaries'
            run('build-production-boundaries', ['g++', *flags, '-I' + str(source / 'src'),
                '-isystem', old / 'install/include/travodds', '-isystem',
                tests / '_deps/googletest-src/googletest/include',
                source / 'test/unit_payloadpool/test_sharedpayloadpool.cpp', library,
                tests / 'lib/libgtest.a', *(['-lgcov'] if args.mode == 'asan' else []),
                '-Wl,-rpath,' + str(old / 'install/lib'), '-o', executable],
                build=True, seconds=90)
            run('production-shared-pool', [executable,
                '--gtest_output=xml:' + str(output / 'production-shared-pool.xml')], env=env)
            root = ET.parse(output / 'production-shared-pool.xml').getroot()
            results['production_tests'] = dict(root.attrib)
            if int(root.attrib['tests']) < 29 or any(int(root.attrib.get(k, 0)) for k in ['failures', 'errors', 'disabled']):
                raise RuntimeError('Unexpected test count/failures in production pool XML')
            if args.repeat > 1:
                run('production-boundaries-repeat', [executable,
                    '--gtest_filter=SharedPoolBoundaries.*:SharedPoolProcesses.NotificationCounterWrap*:SharedPoolProcesses.SignalDelivered*',
                    '--gtest_repeat=' + str(args.repeat),
                    '--gtest_output=xml:' + str(output / 'production-repeat.xml')], env=env,
                    seconds=max(45, args.repeat * 5))
        changed = [str(p.relative_to(source)) for p in tracked_inputs
                   if digest(p) != results['source_sha256'][str(p.relative_to(source))]]
        if changed:
            raise RuntimeError('Inputs changed during checks: ' + repr(changed))
        if any(digest(source / p) != h for p, h in production_snapshot.items()):
            raise RuntimeError('Production inputs changed during checks')
        if library is not None and digest(library) != results['production_library']['sha256']:
            raise RuntimeError('Reused production library changed during checks')
        results['passed'] = True
    except Exception as error:
        results['passed'] = False
        results['error'] = str(error)
        save()
        raise
    save()
    print('Verified results:', output / 'results.json')


if __name__ == '__main__':
    main()
