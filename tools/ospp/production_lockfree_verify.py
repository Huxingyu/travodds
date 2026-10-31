#!/usr/bin/env python3
"""Build the current production backend serially and retain exact test evidence."""
import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import platform
import resource
import re
import shutil
import shlex
import subprocess
import time
import xml.etree.ElementTree as ET
from datetime import datetime, timezone


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--mode', choices=('normal', 'coverage', 'asan', 'tsan'), default='normal')
    parser.add_argument('--scope', choices=('pool', 'integration', 'focused'), default='integration',
                        help='focused: full pool suite plus fully instrumented native DDS progress test')
    parser.add_argument('--core', action='store_true', help='also build and run unit_inside')
    parser.add_argument('--examples', type=int, default=0, metavar='ROUNDS')
    parser.add_argument('--focus-test', action='append', default=[], metavar='[FIXTURE.]NAME',
                        help='additional LoanIntegration or NativeReaderIntegration focused cases')
    parser.add_argument('--baseline', default='489067108405b27afbd369245542f5e88d457f18')
    parser.add_argument('--compiler-memory-mib', type=int, default=768,
                        help='Compiler-only address-space cap; runtime shadow mappings remain unlimited')
    parser.add_argument('--trim-build-cache', action='store_true',
                        help='Discard recorded rebuildable objects after each completed build stage')
    parser.add_argument('--compile-timeout', type=int, default=7200,
                        help='Per-stage compile timeout, allowing low-priority serial builds')
    args = parser.parse_args()
    if args.compiler_memory_mib < 256:
        parser.error('The compiler memory limit must be at least 256 MiB')
    source = Path(__file__).resolve().parents[2]
    output, build = args.output.resolve(), args.build_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    if any(output.iterdir()):
        parser.error('Use a new, empty evidence directory')
    build.mkdir(parents=True, exist_ok=True)
    lock = (build.parent / 'compiler.lock').open('w')
    fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    inputs = sorted(p for directory in ('src', 'include', 'test/unit_payloadpool',
                                      'test/unit_inside', 'example/cpp/data_sharing')
                    for p in (source / directory).rglob('*')
                    if p.is_file() and p.suffix in ('.h', '.hpp', '.cpp', '.txt'))
    inputs += [source / 'CMakeLists.txt', Path(__file__).resolve(),
               source / 'tools/ospp/coverage.py', source / 'tools/ospp/datasharing_examples.py',
               source / 'tools/ospp/native_payload_writes.py']
    record = {'source': str(source), 'mode': args.mode, 'scope': args.scope,
              'started_utc': datetime.now(timezone.utc).isoformat(),
              'nice': os.getpriority(os.PRIO_PROCESS, 0),
              'jobs': 1, 'compiler_memory_mib': args.compiler_memory_mib, 'commands': [],
              'focus_tests': args.focus_test, 'examples_rounds': args.examples,
              'source_sha256': {str(p.relative_to(source)): digest(p) for p in inputs}}
    (output / 'harness.py').write_bytes(Path(__file__).read_bytes())
    env = os.environ.copy()
    if args.mode == 'asan':
        # The independent minimal probe in notification-progress-20261029
        # established that LSan cannot scan under this environment's ptrace.
        env['ASAN_OPTIONS'] = 'detect_leaks=0:halt_on_error=1'
        env['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
        record['leak_check'] = 'not executed (documented ptrace environment limitation)'
    if args.mode == 'tsan':
        env['TSAN_OPTIONS'] = 'halt_on_error=1:exitcode=66'

    def save():
        (output / 'results.json').write_text(json.dumps(record, indent=2) + '\n')

    def trim_cache(stage):
        if not args.trim_build_cache:
            return
        paths = sorted(p for p in build.rglob('*.o') if 'CMakeFiles' in p.parts)
        installed = build / 'install/lib/libtravodds.so.1.1.0'
        duplicate = build / 'library/src/libtravodds.so.1.1.0'
        if installed.is_file() and duplicate.is_file():
            if digest(installed) != digest(duplicate) or installed.stat().st_ino == duplicate.stat().st_ino:
                raise RuntimeError('Unexpected library identity while trimming build cache')
            paths.append(duplicate)
        entries = [{'path': str(p), 'bytes': p.stat().st_size, 'sha256': digest(p)} for p in paths]
        # Coverage notes/data, installed libraries, binaries and GTest archives
        # remain intact. Save hashes before removing any reproducible cache.
        (output / (stage + '-cache.json')).write_text(json.dumps(entries, indent=2) + '\n')
        for p in paths:
            p.unlink()
        record.setdefault('trimmed_cache', {})[stage] = {
            'files': len(entries), 'bytes': sum(p['bytes'] for p in entries)}
        save()

    def limits():
        resource.setrlimit(resource.RLIMIT_AS, (args.compiler_memory_mib * 1024 * 1024,) * 2)
        os.nice(19)
        # If another workload consumes the remaining memory, prefer failing
        # this bounded compiler over evicting an interactive user process.
        try:
            Path('/proc/self/oom_score_adj').write_text('500')
        except OSError:
            pass

    def completed_tests(xml):
        root = ET.parse(xml).getroot()
        attributes = root.attrib
        cases = list(root.iter('testcase'))
        if not cases or int(attributes['tests']) != len(cases) or any(
                int(attributes.get(k, 0)) for k in ('failures', 'errors', 'disabled', 'skipped')):
            raise RuntimeError('Failed, skipped, disabled or empty test suite: ' + str(xml))
        identities = {(case.attrib['classname'], case.attrib['name']) for case in cases}
        if len(identities) != len(cases) or any(
                case.attrib.get('status') != 'run' or case.attrib.get('result') != 'completed' or
                any(case.find(tag) is not None for tag in ('failure', 'error', 'skipped'))
                for case in cases):
            raise RuntimeError('Incomplete or duplicate test results: ' + str(xml))
        return attributes

    def run(name, command, compiling=False, seconds=300):
        if compiling and shutil.disk_usage(output).free < 500 * 1024 * 1024:
            raise RuntimeError('Less than 500 MiB disk free; existing artifacts retained')
        command = ['timeout', '--kill-after=5s', str(seconds) + 's', '/usr/bin/time',
                   '-v', '-o', str(output / (name + '-resources.txt')), *map(str, command)]
        started = time.monotonic()
        with (output / (name + '.log')).open('w') as log:
            result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT,
                                    env=env, preexec_fn=limits if compiling else None)
        record['commands'].append({'name': name, 'command': command,
                                   'exit_code': result.returncode,
                                   'seconds': time.monotonic() - started})
        save()
        print(name, 'exit', result.returncode, flush=True)
        if result.returncode:
            raise RuntimeError(name + ' failed; see retained log')

    try:
        # Aggressive GCC collection trades compile time for a smaller live
        # heap on the shared 2 GiB VM. It does not change program semantics.
        gcc_gc = ' --param=ggc-min-expand=15 --param=ggc-min-heapsize=16384'
        flags = '-O1 -g0 -DNDEBUG'
        if args.mode == 'coverage':
            # DDS exercises concurrent paths. Atomic profile updates avoid
            # lost arc increments and impossible negative inferred counts.
            flags = '-O0 -g0 -DNDEBUG --coverage -fprofile-update=atomic'
        flags += gcc_gc
        if args.mode in ('asan', 'tsan'):
            flags += ' -fno-omit-frame-pointer -fsanitize=' + (
                'address,undefined' if args.mode == 'asan' else 'thread')
        install = build / 'install'
        cmake_flags = ['-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_CXX_FLAGS_RELEASE=' + flags]
        if args.mode in ('asan', 'tsan'):
            cmake_flags += ['-DCMAKE_EXE_LINKER_FLAGS=-no-pie']
        run('configure-library', ['cmake', '-S', source, '-B', build / 'library',
            '-DCMAKE_INSTALL_PREFIX=' + str(install), *cmake_flags], True)
        run('build-library', ['cmake', '--build', build / 'library', '--parallel', '1'], True, args.compile_timeout)
        run('install-library', ['cmake', '--install', build / 'library'])
        library = install / 'lib/libtravodds.so.1.1.0'
        record['library'] = {'path': str(library), 'sha256': digest(library)}
        trim_cache('library')
        tests = build / 'tests'
        test_flags = list(cmake_flags)
        if args.mode in ('asan', 'tsan'):
            # The large integration translation unit exceeds this VM's
            # bounded compiler memory with sanitizer optimization enabled.
            # Keep the production library at O1; instrument the test drivers
            # at O0 so both remain inside the same 768 MiB compiler limit.
            test_flags = [flag.replace('-O1 -g0', '-O0 -g0') for flag in test_flags]
        run('configure-tests', ['cmake', '-S', source / 'test/unit_payloadpool', '-B', tests,
            '-DCMAKE_PREFIX_PATH=' + str(install),
            # Sanitizer runtimes need a huge virtual shadow mapping. Defer
            # GoogleTest executable discovery until after the compiler-only
            # address-space limit has been released.
            '-DCMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE=PRE_TEST', *test_flags], True)
        targets = ['sharedpayloadpool_test']
        if args.scope == 'integration':
            targets.append('payloadpool_test')
        if args.scope == 'focused':
            targets += ['gtest_main', 'native_key_admission_test',
                        'native_initialization_test', 'native_payload_writes_test',
                        'reader_payloadpool_test']
        run('build-tests', ['cmake', '--build', tests, '--parallel', '1', '--target', *targets], True, args.compile_timeout)
        trim_cache('tests')
        prefix = ['setarch', platform.machine(), '-R'] if args.mode == 'tsan' else []
        test_targets = [target for target in targets if target != 'gtest_main']
        if args.scope == 'focused':
            # Reuse the exact existing fixture and case, without compiling
            # 3,000 unrelated test lines above this VM's memory budget. The
            # full source hash and generated driver are retained as evidence.
            text = (source / 'test/unit_payloadpool/test_loan.cpp').read_text()
            prefix_end = text.index('TEST_F(LoanIntegration, KeyedDeadlineDoesNotInventAnInstanceBeforeFirstWrite)')
            selected = ['NativeDdsWritesAndLoansProgressWhileThePoolControlMutexIsHeld', *args.focus_test]
            cases = []
            helpers = ''
            for selector in selected:
                fixture, name = selector.split('.', 1) if '.' in selector else ('LoanIntegration', selector)
                if fixture not in ('LoanIntegration', 'NativeReaderIntegration'):
                    raise RuntimeError('Unsupported focused fixture: ' + fixture)
                if fixture == 'NativeReaderIntegration' and not helpers:
                    begin = text.index('namespace {\nstruct NativeDelivery {')
                    end = text.index('\nTEST_F(NativeReaderIntegration,', begin)
                    helpers = text[begin:end]
                    if 'TEST_F(' in helpers or not helpers.rstrip().endswith('};\n}'):
                        raise RuntimeError('NativeReaderIntegration helper boundary changed')
                case_begin = text.index('TEST_F(' + fixture + ', ' + name + ')')
                # In this fixture each case ends with a brace in column zero;
                # nested scopes are indented. Fail rather than include another
                # case or silently compile a different test after source edits.
                end = re.search(r'^}\s*$', text[case_begin:], re.MULTILINE)
                if not end:
                    raise RuntimeError('No case boundary for ' + name)
                case = text[case_begin:case_begin + end.end()]
                if case.count('TEST_F(') != 1:
                    raise RuntimeError('Ambiguous case boundary for ' + name)
                cases.append(case)
            driver = output / 'native-dds-progress.cpp'
            driver.write_text(text[:prefix_end] + '\n}\n' + helpers + '\n'.join(cases) + '\n')
            record['focused_driver_sha256'] = digest(driver)
            executable = tests / 'native_dds_progress'
            includes = ['include', 'src', 'thirdparty', 'thirdparty/tao', 'thirdparty/asio-1.13.0/include']
            run('build-native-dds-progress', ['g++', '-std=c++17', '-O0', '-g0', '-pthread',
                *shlex.split(flags.replace('-O1 -g0', '-O0 -g0')), '-fno-pie', '-no-pie',
                *['-I' + str(source / directory) for directory in includes],
                '-I' + str(tests / '_deps/googletest-src/googletest/include'), driver,
                library, tests / 'lib/libgtest_main.a', tests / 'lib/libgtest.a',
                '-Wl,-rpath,' + str(install / 'lib'), '-o', executable], True, 300)
            test_targets.append('native_dds_progress')
        for target in test_targets:
            xml = output / (target + '.xml')
            run(target, [*prefix, tests / target, '--gtest_output=xml:' + str(xml)])
            attributes = completed_tests(xml)
            record[target] = attributes
            if int(attributes['tests']) == 0 or any(int(attributes.get(k, 0))
                    for k in ('failures', 'errors', 'disabled')):
                raise RuntimeError(target + ' has failures, disabled tests or empty coverage')
        if args.scope in ('integration', 'focused'):
            guard_target = 'payloadpool_test' if args.scope == 'integration' else 'native_payload_writes_test'
            proof = output / 'native-payload-writes.json'
            run('verify-native-payload-writes', ['python3', source / 'tools/ospp/native_payload_writes.py',
                '--xml', output / (guard_target + '.xml'),
                '--log', output / (guard_target + '.log'), '--output', proof])
            record['native_payload_writes'] = json.loads(proof.read_text())
            if record['native_payload_writes'].get('passed') is not True:
                raise RuntimeError('Native payload write proof is incomplete')
        if args.core:
            core = build / 'core'
            run('configure-core', ['cmake', '-S', source / 'test/unit_inside', '-B', core,
                '-DCMAKE_PREFIX_PATH=' + str(install),
                '-DBUILD_GMOCK=OFF', '-DCMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE=PRE_TEST',
                *test_flags], True)
            run('build-core', ['cmake', '--build', core, '--parallel', '1'], True, args.compile_timeout)
            trim_cache('core')
            xml = output / 'core.xml'
            run('core', [*prefix, core / 'unit_test', '--gtest_output=xml:' + str(xml)], seconds=650)
            record['core'] = completed_tests(xml)
            if int(record['core']['tests']) == 0 or any(int(record['core'].get(k, 0))
                    for k in ('failures', 'errors', 'disabled')):
                raise RuntimeError('Core suite failed or empty')
        if args.examples:
            examples = build / 'examples'
            run('configure-examples', ['cmake', '-S', source / 'example/cpp/data_sharing',
                '-B', examples, '-DCMAKE_PREFIX_PATH=' + str(install), *cmake_flags], True)
            run('build-examples', ['cmake', '--build', examples, '--parallel', '1'], True, args.compile_timeout)
            trim_cache('examples')
            run('examples', ['python3', source / 'tools/ospp/datasharing_examples.py',
                '--build', examples, '--output', output / 'examples', '--rounds', args.examples],
                seconds=max(300, args.examples * 3 * 40))
            record['examples'] = json.loads((output / 'examples/summary.json').read_text())
        if args.mode == 'coverage':
            run('coverage', ['python3', source / 'tools/ospp/coverage.py', '--source', source,
                '--build-root', build, '--output', output / 'coverage',
                '--baseline', args.baseline, '--minimum', '70'], seconds=300)
            record['coverage'] = json.loads((output / 'coverage/summary.json').read_text())
        changed = [str(p.relative_to(source)) for p in inputs
                   if digest(p) != record['source_sha256'][str(p.relative_to(source))]]
        if changed:
            raise RuntimeError('Source changed while verifying: ' + repr(changed))
        if digest(library) != record['library']['sha256']:
            raise RuntimeError('Library changed while verifying')
        record['passed'] = True
    except Exception as error:
        record['passed'], record['error'] = False, str(error)
        save()
        raise
    record['completed_utc'] = datetime.now(timezone.utc).isoformat()
    save()
    print('Verified production backend:', output / 'results.json', flush=True)


if __name__ == '__main__':
    main()
