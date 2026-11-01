#!/usr/bin/env python3
"""Strict Modal build worker; importing this file performs no work.

Only modal_worker.py calls execute(). All outputs are new files below ROOT.
No old artifact, local credential, network checkout, test retry, test skip,
coverage collection, or local build is part of this worker.
"""
from __future__ import annotations

import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import resource
import shlex
import shutil
import signal
import subprocess
import tarfile
import time
import traceback
import xml.etree.ElementTree as ET


ROOT = Path('/home/ubuntu/project/travodds/artifacts/datasharing-final-20261101')
SOURCE = Path('/home/ubuntu/project/travodds/.worktrees/datasharing-progress')
MODES = ('plain', 'coverage', 'asan', 'tsan')
COMMON = ['-g1', '-DNDEBUG', '-I' + str(SOURCE / 'include'), '--param=ggc-min-expand=15',
          '--param=ggc-min-heapsize=16384']
DIAGNOSTICS = ('WARNING: ThreadSanitizer:', 'ERROR: ThreadSanitizer:',
    'FATAL: ThreadSanitizer:', 'ERROR: AddressSanitizer:', 'runtime error:',
    'UndefinedBehaviorSanitizer:')
REQUIRED_SANITIZER = {'plain': (), 'coverage': ('__gcov_',),
    'asan': ('__asan_', '__ubsan_'), 'tsan': ('__tsan_',)}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'),
                      ensure_ascii=False).encode()


def digest(path):
    value = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def write(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + '.partial')
    temporary.write_text(json.dumps(value, indent=2, ensure_ascii=False) + '\n')
    temporary.replace(path)


def source_map():
    return {str(path.relative_to(SOURCE)): digest(path)
            for path in sorted(SOURCE.rglob('*')) if path.is_file()}


def frozen_source(archive, manifest):
    require(digest(archive) == manifest['source_archive_sha256'], 'Source archive SHA256 mismatch')
    expected = manifest['source_sha256']
    require(isinstance(expected, dict) and expected, 'Missing complete frozen source map')
    identity = hashlib.sha256(canonical(expected)).hexdigest()
    require(identity == manifest['canonical_source_identity_sha256'], 'Noncanonical source identity')
    require(re.fullmatch(r'[0-9a-f]{40}', manifest['implementation_commit']) is not None,
            'Expected full committed source revision')
    if not SOURCE.exists():
        SOURCE.mkdir(parents=True)
        seen = set()
        with tarfile.open(archive, 'r:gz') as handle:
            for member in handle:
                name = member.name
                while name.startswith('./'):
                    name = name[2:]
                if not name:
                    continue
                path = Path(name)
                require(not path.is_absolute() and '..' not in path.parts,
                        'Unsafe source archive member')
                destination = SOURCE / path
                if member.isdir():
                    destination.mkdir(parents=True, exist_ok=True)
                    continue
                require(member.isfile() and name in expected and name not in seen,
                        'Unexpected, duplicate or nonregular source archive member: ' + name)
                seen.add(name)
                stream = handle.extractfile(member)
                require(stream is not None, 'Unreadable source archive member')
                destination.parent.mkdir(parents=True, exist_ok=True)
                with destination.open('xb') as output:
                    shutil.copyfileobj(stream, output)
                destination.chmod(member.mode & 0o777)
        require(seen == set(expected), 'Source archive omitted frozen files')
    require(not any(path.is_symlink() for path in SOURCE.rglob('*')), 'Source contains symlinks')
    actual = source_map()
    require(actual == expected, 'Extracted source differs from frozen input manifest')
    return identity


def processes():
    result = {}
    for path in Path('/proc').iterdir():
        if not path.name.isdigit():
            continue
        try:
            fields = (path / 'stat').read_text().rsplit(')', 1)[1].split()
            result[int(path.name)] = {'state': fields[0], 'parent': int(fields[1]),
                                      'group': int(fields[2])}
        except (OSError, ValueError, IndexError):
            pass
    return result


def descendants(pid, snapshot):
    found = {pid}
    while True:
        more = {child for child, row in snapshot.items() if row['parent'] in found}
        if more <= found:
            return found
        found.update(more)


def kill_tree(pid, tracked):
    snapshot = processes()
    targets = tracked | descendants(pid, snapshot)
    targets.update(child for child, row in snapshot.items() if row['group'] == pid)
    for child in targets - {pid}:
        try:
            os.kill(child, signal.SIGKILL)
        except ProcessLookupError:
            pass
    try:
        os.killpg(pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    return sorted(targets)


class Worker:
    def __init__(self, mode, manifest, worker_hash, wrapper_hash, context):
        self.mode, self.manifest = mode, manifest
        self.start = time.monotonic()
        self.output = ROOT / 'remote' / mode
        self.output.mkdir(parents=True, exist_ok=False)
        self.logs = self.output / 'logs'
        self.logs.mkdir()
        self.build = ROOT / 'build' / mode
        require(not self.build.exists(), 'Build directory already exists; never overwrite an attempt')
        self.build.mkdir(parents=True)
        self.install = self.build / 'install'
        self.counter = ROOT / 'counters' / 'modal'
        self.env = dict(os.environ, LC_ALL='C', LANG='C',
                        LD_LIBRARY_PATH=str(self.install / 'lib'))
        for name in ('GTEST_FILTER', 'GTEST_REPEAT', 'GTEST_TOTAL_SHARDS', 'GTEST_SHARD_INDEX',
                     'LD_PRELOAD', 'GCOV_PREFIX', 'GCOV_PREFIX_STRIP'):
            self.env.pop(name, None)
        self.env.update(TSAN_OPTIONS='halt_on_error=1:exitcode=66:second_deadlock_stack=1',
            ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
            UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
        if mode == 'coverage':
            require(not self.counter.exists(), 'Coverage prefix must start new and empty')
            self.counter.mkdir(parents=True)
            self.env.update(GCOV_PREFIX=str(self.counter), GCOV_PREFIX_STRIP='0')
        self.ship = set()
        self.labels = set()
        self.record = {'schema': 1, 'mode': mode, 'started_utc': utc(),
            'modal_phase_passed': False, 'acceptance_passed': False,
            'implementation_commit': manifest['implementation_commit'],
            'canonical_source_identity_sha256': manifest['canonical_source_identity_sha256'],
            'source_archive_sha256': manifest['source_archive_sha256'],
            'source_manifest_sha256': None, 'source_sha256': manifest['source_sha256'],
            'source': str(SOURCE), 'build': str(self.build),
            'worker_sha256': worker_hash, 'modal_wrapper_sha256': wrapper_hash,
            'context': context, 'resources': {'cpu': 2, 'memory_mib': 4096,
                'parallel_build_jobs': 2, 'function_timeout_seconds': 900,
                'worker_command_deadline_seconds': 780, 'retries': 0,
                'nice': os.getpriority(os.PRIO_PROCESS, 0)},
            'commands': [], 'expected_tests': {}, 'elf': {},
            'shm_before': sorted(path.name for path in Path('/dev/shm').iterdir()),
            'shm_after': None, 'shm_added': None,
            'environment_policy': {'recorded': {'LD_LIBRARY_PATH': self.env['LD_LIBRARY_PATH'],
                'TSAN_OPTIONS': self.env['TSAN_OPTIONS'], 'ASAN_OPTIONS': self.env['ASAN_OPTIONS'],
                'UBSAN_OPTIONS': self.env['UBSAN_OPTIONS']},
                'coverage': {key: self.env.get(key) for key in ('GCOV_PREFIX', 'GCOV_PREFIX_STRIP')},
                'gtest_filter_repeat_and_sharding_cleared': True,
                'setarch_used': False, 'secrets_requested': False},
            'limitations': ['This worker completes only the Modal phase; all shared-pool tests and core tests are assigned to the real-kernel local phase.',
                'ASan/UBSan disables LSan; no leak-scan claim.',
                'Hardware DRAM writes are not measured by native payload guards.']}
        self.save()

    def save(self):
        write(self.output / 'results.json', self.record)

    def run(self, label, argv, timeout=600, cwd=None, expect_zero=True):
        require(label not in self.labels, 'Command labels must be unique')
        self.labels.add(label)
        remaining = 780 - (time.monotonic() - self.start)
        require(remaining > 1, 'Worker command deadline exhausted; no more paid work')
        duration = min(timeout, remaining)
        log, usage = self.logs / (label + '.log'), self.logs / (label + '-resources.txt')
        actual = ['/usr/bin/time', '-v', '-o', str(usage), *map(str, argv)]
        started_utc = utc()
        started = time.monotonic()
        with log.open('xb') as output:
            process = subprocess.Popen(actual, cwd=cwd or self.build, env=self.env,
                stdout=output, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                start_new_session=True)
            tracked = {process.pid}
            timed_out = False
            killed = []
            while process.poll() is None:
                tracked.update(descendants(process.pid, processes()))
                if time.monotonic() - started > duration:
                    timed_out = True
                    killed = kill_tree(process.pid, tracked)
                    break
                time.sleep(0.1)
            status = process.wait()
            snapshot = processes()
            survivors = sorted(pid for pid in tracked if pid != process.pid and pid in snapshot
                               and snapshot[pid]['state'] != 'Z')
            survivors += sorted(pid for pid, row in snapshot.items()
                                if row['group'] == process.pid and row['state'] != 'Z'
                                and pid not in survivors)
            if survivors:
                killed = sorted(set(killed) | set(kill_tree(process.pid, set(survivors))))
        row = {'label': label, 'argv': list(map(str, argv)), 'actual_argv': actual,
            'cwd': str(cwd or self.build), 'started_utc': started_utc, 'exit_code': status,
            'seconds': round(time.monotonic() - started, 6), 'timeout_seconds': duration,
            'timed_out': timed_out, 'unexpected_survivors': survivors, 'killed_pids': killed,
            'log': str(log), 'log_sha256': digest(log), 'resources': str(usage),
            'resources_sha256': digest(usage) if usage.exists() else None}
        self.record['commands'].append(row)
        self.save()
        print(json.dumps({'mode': self.mode, 'label': label, 'exit': status,
                          'seconds': row['seconds']}, sort_keys=True), flush=True)
        require(not timed_out and not survivors, 'Command timed out or leaked children: ' + label)
        if expect_zero:
            require(status == 0, 'Command failed: ' + label)
        return log.read_text(errors='strict')

    def compiler_fingerprint(self):
        result = {}
        for label, argv in {
            'gxx-version': ['g++', '--version'], 'gxx-fullversion': ['g++', '-dumpfullversion'],
            'gcc-version': ['gcc', '--version'], 'gcov-version': ['gcov', '--version'],
            'gcov-tool-version': ['gcov-tool', '--version'], 'cmake-version': ['cmake', '--version'],
            'linker-version': ['ld', '--version'], 'kernel': ['uname', '-a'],
            'packages': ['dpkg-query', '-W', '-f=${Package}\t${Version}\n',
                'g++', 'g++-13', 'gcc-13', 'libstdc++6', 'libgcc-s1', 'libc6',
                'libasan8', 'libubsan1', 'libtsan2', 'binutils', 'cmake'],
        }.items():
            result[label] = self.run('fingerprint-' + label, argv, timeout=30)
        require(result['gxx-fullversion'].strip().startswith('13.3.'), 'g++ must be version 13.3.x')
        for path in ('/usr/bin/g++', '/usr/bin/gcc', '/usr/bin/gcov', '/usr/bin/gcov-tool',
                     '/usr/bin/ld', '/usr/bin/addr2line', '/etc/os-release'):
            resolved = Path(path).resolve()
            result[path] = {'resolved': str(resolved), 'sha256': digest(resolved)}
        os_release = Path('/etc/os-release').read_text()
        require('VERSION_ID="24.04"' in os_release, 'Expected Ubuntu 24.04 image')
        self.record['compiler_and_platform'] = result
        self.save()

    def configure_and_build(self):
        extra = []
        if self.mode == 'coverage':
            extra = ['--coverage', '-fprofile-update=atomic']
        elif self.mode in ('asan', 'tsan'):
            extra = ['-fno-omit-frame-pointer', '-fsanitize=' +
                     ('address,undefined' if self.mode == 'asan' else 'thread')]
        library_flags = ['-O1' if self.mode in ('asan', 'tsan') else '-O0', *COMMON, *extra]
        test_flags = ['-O0', *COMMON, *extra]
        if self.mode in ('asan', 'tsan'):
            test_flags.append('-fno-pie')
        linker = '-no-pie' if self.mode in ('plain', 'asan', 'tsan') else ''
        self.record['build_flags'] = {'library': library_flags, 'tests_and_examples': test_flags,
                                      'executable_linker_flags': linker}
        self.run('configure-library', ['cmake', '-S', SOURCE, '-B', self.build / 'library',
            '-DCMAKE_BUILD_TYPE=Release', '-DBUILD_SHARED_LIBS=ON', '-DSECURITY=OFF',
            '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON', '-DCMAKE_CXX_FLAGS_RELEASE=' + ' '.join(library_flags),
            '-DCMAKE_EXE_LINKER_FLAGS=' + linker, '-DCMAKE_INSTALL_PREFIX=' + str(self.install)])
        self.run('build-library', ['cmake', '--build', self.build / 'library',
                                  '--parallel', '2', '--target', 'install'])
        self.run('configure-modules', ['cmake', '-S', SOURCE / 'test/unit_payloadpool',
            '-B', self.build / 'tests', '-DCMAKE_BUILD_TYPE=Release',
            '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON', '-DCMAKE_CXX_FLAGS_RELEASE=' + ' '.join(test_flags),
            '-DCMAKE_EXE_LINKER_FLAGS=' + linker, '-DCMAKE_PREFIX_PATH=' + str(self.install)])
        self.run('build-modules', ['cmake', '--build', self.build / 'tests', '--parallel', '2',
            '--target', 'payloadpool_test', 'sharedpayloadpool_test'])
        if self.mode in ('plain', 'coverage'):
            self.run('configure-core', ['cmake', '-S', SOURCE / 'test/unit_inside',
                '-B', self.build / 'core', '-DCMAKE_BUILD_TYPE=Release',
                '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON', '-DCMAKE_CXX_FLAGS_RELEASE=' + ' '.join(test_flags),
                '-DCMAKE_EXE_LINKER_FLAGS=' + linker, '-DCMAKE_PREFIX_PATH=' + str(self.install)])
            self.run('build-core', ['cmake', '--build', self.build / 'core',
                                    '--parallel', '2', '--target', 'unit_test'])
        if self.mode == 'coverage':
            self.run('configure-examples', ['cmake', '-S', SOURCE / 'example/cpp/data_sharing',
                '-B', self.build / 'examples', '-DCMAKE_BUILD_TYPE=Release',
                '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON', '-DCMAKE_CXX_FLAGS_RELEASE=' + ' '.join(test_flags),
                '-DCMAKE_EXE_LINKER_FLAGS=' + linker, '-DCMAKE_PREFIX_PATH=' + str(self.install)])
            self.run('build-examples', ['cmake', '--build', self.build / 'examples', '--parallel', '2',
                '--target', 'datasharing_loan', 'datasharing_keyed', 'datasharing_mixed'])

    def binaries(self):
        result = {'payloadpool_test': self.build / 'tests/payloadpool_test',
                  'sharedpayloadpool_test': self.build / 'tests/sharedpayloadpool_test'}
        if self.mode in ('plain', 'coverage'):
            result['unit_test'] = self.build / 'core/unit_test'
        if self.mode == 'coverage':
            result.update({name: self.build / 'examples' / name
                           for name in ('datasharing_loan', 'datasharing_keyed', 'datasharing_mixed')})
        return result

    def list_tests(self):
        for label, path in self.binaries().items():
            if label.startswith('datasharing_'):
                continue
            text = self.run('list-' + label, [path, '--gtest_list_tests'], timeout=90)
            current, identities = None, []
            for raw in text.splitlines():
                line = raw.split('#', 1)[0].rstrip()
                if re.fullmatch(r'[A-Za-z0-9_./-]+\.', line):
                    current = line[:-1]
                elif current and re.fullmatch(r'\s+[A-Za-z0-9_./-]+', line):
                    identities.append([current, line.strip()])
            require(identities and len(identities) == len({tuple(item) for item in identities}),
                    'Empty or duplicate test listing: ' + label)
            minimum = {'payloadpool_test': 256, 'sharedpayloadpool_test': 49, 'unit_test': 247}[label]
            require(len(identities) >= minimum, 'Frozen program omitted existing cases: ' + label)
            self.record['expected_tests'][label] = {'binary': str(path), 'count': len(identities),
                'case_identities': sorted(identities),
                'assigned_environment': 'modal' if label == 'payloadpool_test' else 'local-real-kernel',
                'reason': 'portable aggregate' if label == 'payloadpool_test' else
                    'full suite assigned to real kernel; gVisor lacks proc syscall/wchan' if label == 'sharedpayloadpool_test' else
                    'full suite assigned to real kernel; gVisor lacks IPv6 and changes network timing'}
        self.record['local_native_proof'] = {'binary': str(self.binaries()['payloadpool_test']),
            'filter': 'NativePayloadWrites.*', 'expected_count': 4,
            'assigned_environment': 'local-real-kernel',
            'separate_xml_and_raw_log_required': True}
        if self.mode == 'coverage':
            self.record['local_examples'] = {'rounds': 3, 'programs':
                [str(self.build / 'examples' / ('datasharing_' + name)) for name in ('loan', 'keyed', 'mixed')],
                'entrypoint': str(SOURCE / 'tools/ospp/datasharing_examples.py'),
                'assigned_environment': 'local-real-kernel'}
        self.save()

    def run_payload(self):
        binary = self.binaries()['payloadpool_test']
        xml_path = self.output / 'payloadpool_test.xml'
        text = self.run('payloadpool_test', [binary, '--gtest_output=xml:' + str(xml_path)], timeout=180)
        require(not any(item in text for item in DIAGNOSTICS), 'Sanitizer diagnostic in aggregate run')
        root = ET.parse(xml_path).getroot()
        cases = list(root.iter('testcase'))
        identities = sorted([case.get('classname'), case.get('name')] for case in cases)
        expected = self.record['expected_tests']['payloadpool_test']
        require(identities == expected['case_identities'] and len(cases) == int(root.get('tests', '-1')),
                'Actual payload identities differ from complete listing')
        require(all(int(root.get(key, '0')) == 0 for key in ('failures', 'errors', 'disabled', 'skipped')),
                'Failed, disabled or skipped aggregate tests')
        require(all(case.get('status') == 'run' and case.get('result') == 'completed' and
                    not any(case.find(key) is not None for key in ('failure', 'error', 'skipped'))
                    for case in cases), 'A payload identity did not complete')
        self.record['payloadpool_result'] = {'xml': str(xml_path), 'sha256': digest(xml_path),
            'attributes': dict(root.attrib), 'case_identities': identities, 'passed': True}
        proof = self.output / 'native-payload-writes.json'
        self.run('verify-native-payload', ['python3', SOURCE / 'tools/ospp/native_payload_writes.py',
            '--xml', xml_path, '--log', self.logs / 'payloadpool_test.log', '--output', proof], timeout=30)
        self.record['native_payload_proof'] = {'path': str(proof), 'sha256': digest(proof),
                                              'summary': json.loads(proof.read_text())}
        self.save()

    def elf_and_runtimes(self):
        private = self.install / 'lib/libtravodds.so.1.1.0'
        require(private.is_file(), 'Missing installed private library')
        entries = {'libtravodds': private, **self.binaries()}
        runtimes = {}
        for label, path in entries.items():
            dynamic = self.run('elf-dynamic-' + label, ['readelf', '-d', path], timeout=30)
            notes = self.run('elf-notes-' + label, ['readelf', '-n', path], timeout=30)
            symbols = self.run('elf-symbols-' + label, ['nm', '-D', path], timeout=30)
            ldd = self.run('elf-ldd-' + label, ['ldd', path], timeout=30)
            require('not found' not in ldd, 'Unresolved ELF dependency: ' + label)
            require(all(token in symbols for token in REQUIRED_SANITIZER[self.mode]),
                    'Missing expected instrumentation symbols: ' + label)
            if label != 'libtravodds':
                match = re.search(r'libtravodds\.so\S*\s+=>\s+(\S+)', ldd)
                require(match and Path(match.group(1)).resolve() == private.resolve(),
                        'ELF did not resolve this run private library: ' + label)
            dependencies = []
            for line in ldd.splitlines():
                match = re.search(r'=>\s+(/\S+)', line) or re.match(r'\s*(/\S+)', line)
                if not match:
                    continue
                alias = Path(match.group(1))
                resolved = alias.resolve()
                dependencies.append({'requested_name': alias.name, 'path': str(alias),
                                     'resolved': str(resolved), 'sha256': digest(resolved)})
                if resolved != private.resolve():
                    runtimes[str(alias)] = resolved
            build_id = re.search(r'Build ID:\s+(\S+)', notes)
            self.record['elf'][label] = {'path': str(path), 'sha256': digest(path),
                'bytes': path.stat().st_size, 'build_id': build_id.group(1) if build_id else None,
                'dynamic_section_log_sha256': digest(self.logs / ('elf-dynamic-' + label + '.log')),
                'dynamic_symbols_log_sha256': digest(self.logs / ('elf-symbols-' + label + '.log')),
                'instrumentation_tokens': list(REQUIRED_SANITIZER[self.mode]),
                'ldd': ldd, 'dependencies': dependencies}
            self.ship.add(path)
        # Reference copies pin every loaded system ELF. Only the portable set
        # belongs on LD_LIBRARY_PATH; glibc/loader reference copies do not.
        runtime_dir = ROOT / 'runtime' / self.mode
        runtime_manifest = []
        for alias, resolved in sorted(runtimes.items()):
            portable = Path(alias).name.startswith(('libasan.', 'libubsan.', 'libtsan.',
                                                    'libstdc++.', 'libgcc_s.'))
            folder = runtime_dir / ('portable' if portable else 'reference')
            folder.mkdir(parents=True, exist_ok=True)
            target = folder / resolved.name
            if target.exists():
                require(digest(target) == digest(resolved), 'Conflicting runtime library basename')
            else:
                shutil.copy2(resolved, target)
            alias_path = folder / Path(alias).name
            if alias_path != target and not alias_path.exists():
                alias_path.symlink_to(target.name)
            runtime_manifest.append({'system_alias': alias, 'system_resolved': str(resolved),
                'copied': str(target), 'copied_alias': str(alias_path), 'sha256': digest(target),
                'use_on_ld_library_path': portable})
            self.ship.update((target, alias_path))
        self.record['runtime_libraries'] = runtime_manifest
        self.record['local_ld_library_path'] = [str(self.install / 'lib'), str(runtime_dir / 'portable')]
        self.save()

    def build_provenance(self):
        records, dependencies, notes, aliases = [], {}, [], []
        for path in sorted(self.build.rglob('*')):
            if not path.is_file() or path.is_symlink():
                continue
            name = path.name
            if name in ('flags.make', 'link.txt', 'compile_commands.json', 'CMakeCache.txt') or name.endswith('.o.d'):
                self.ship.add(path)
                records.append({'path': str(path), 'sha256': digest(path)})
            if name.endswith('.o.d'):
                text = path.read_text().replace('\\\n', ' ')
                require(':' in text, 'Malformed compiler dependency file')
                names = shlex.split(text.split(':', 1)[1])
                for token in names:
                    dependency = Path(token)
                    require(dependency.is_absolute(), 'Relative compiler dependency needs explicit cwd mapping')
                    dependency = dependency.resolve()
                    require(dependency.is_file(), 'Missing compiler dependency: ' + str(dependency))
                    dependencies[str(dependency)] = digest(dependency)
            if name.endswith('.gcno'):
                self.ship.add(path)
                notes.append({'path': str(path), 'sha256': digest(path),
                    'official_collector_excluded_dependency': '_deps' in path.parts})
        for folder in (self.build / 'tests/lib', self.build / 'core/lib'):
            if folder.exists():
                for path in sorted(folder.glob('*.a')):
                    self.ship.add(path)
                    records.append({'path': str(path), 'sha256': digest(path), 'role': 'gtest archive'})
        installed_api_dependencies = sorted(name for name in dependencies
            if Path(name).is_relative_to(self.install / 'include/travodds'))
        source_api_dependencies = {name: value for name, value in dependencies.items()
                                   if Path(name).is_relative_to(SOURCE / 'include')}
        require(source_api_dependencies, 'No frozen API headers found in compiler dependencies')
        require(not installed_api_dependencies,
                'API headers resolved through installed copies despite explicit frozen-source -I: ' +
                repr(installed_api_dependencies))
        header_root = self.install / 'include/travodds'
        require(header_root.is_dir(), 'Missing installed header provenance')
        for path in sorted(header_root.rglob('*')):
            if not path.is_file():
                continue
            source = SOURCE / 'include' / path.relative_to(header_root)
            require(source.is_file() and digest(path) == digest(source), 'Installed header differs from frozen source')
            aliases.append({'installed': str(path), 'source': str(source), 'sha256': digest(path)})
            self.ship.add(path)
        write(self.output / 'build-provenance.json', {'records': records,
            'compiler_dependency_sha256': dependencies, 'notes': notes,
            'installed_header_source_aliases': aliases,
            'api_header_resolution': {'source_include_sha256': source_api_dependencies,
                                     'installed_include_dependencies': installed_api_dependencies,
                                     'explicit_source_include_flag': '-I' + str(SOURCE / 'include')},
            'notes_policy': 'All notes produced by the selected aggregate targets, including zero-data TUs. GTest _deps notes are retained but excluded by the unchanged official collector.',
            'header_alias_policy': 'Byte-identical provenance only. No source paths or raw gcov JSON rewritten by this worker.'})
        counters = []
        if self.mode == 'coverage':
            require(notes, 'Coverage build produced no notes')
            outside = list(self.build.rglob('*.gcda'))
            require(not outside, 'Profile counters escaped the dedicated GCOV_PREFIX')
            for path in sorted(self.counter.rglob('*.gcda')):
                self.ship.add(path)
                counters.append({'path': str(path), 'sha256': digest(path), 'bytes': path.stat().st_size})
            require(counters, 'Coverage payload execution produced no runtime counters')
        self.record['build_provenance'] = {'path': str(self.output / 'build-provenance.json'),
            'sha256': digest(self.output / 'build-provenance.json'),
            'notes_count': len(notes), 'official_notes_count': sum(not row['official_collector_excluded_dependency'] for row in notes),
            'compiler_dependency_files': len(dependencies), 'installed_header_aliases': len(aliases)}
        self.record['raw_coverage_counters'] = counters
        self.save()

    def finish_shm(self):
        before = set(self.record['shm_before'])
        added = {path.name for path in Path('/dev/shm').iterdir()} - before
        self.record.setdefault('shm_after_tests', sorted(path.name for path in Path('/dev/shm').iterdir()))
        self.record.setdefault('shm_created_by_mode', sorted(added))
        # The container is private to this worker. DDS global UUID/watchdog
        # objects intentionally survive a process; remove only these exact new
        # objects, after every test child has exited. Never suppress DS leaks.
        allowed = {'TravoddsGlobalUUIDSharedMemory', 'sem.TravoddsGlobalUUIDMutex',
                   'TravoddsWatchDogSharedMemory', 'sem.TravoddsWatchDogUUIDMutex'}
        cleanup = self.record.setdefault('shm_global_cleanup', [])
        for name in sorted(added & allowed):
            path = Path('/dev/shm') / name
            require(path.is_file() and not path.is_symlink(), 'Unexpected global SHM file type')
            info = path.stat()
            cleanup.append({'name': name, 'inode': info.st_ino, 'device': info.st_dev,
                            'bytes': info.st_size, 'reason': 'private-container global DDS singleton after process exit'})
            path.unlink()
        self.record['shm_global_cleanup'] = cleanup
        self.record['shm_after'] = sorted(path.name for path in Path('/dev/shm').iterdir())
        self.record['shm_added'] = sorted(set(self.record['shm_after']) - before)
        self.save()
        require(not self.record['shm_added'], 'New shared-memory residue; do not delete or hide it')

    def package(self):
        self.ship.update(path for path in self.output.rglob('*') if path.is_file())
        # Failed attempts retain available binaries, notes and compile records
        # too; a failure before provenance collection must remain reproducible.
        for path in self.build.rglob('*'):
            if path.is_file() and (path.name in ('flags.make', 'link.txt', 'compile_commands.json',
                    'CMakeCache.txt') or path.name.endswith(('.o.d', '.gcno'))):
                self.ship.add(path)
        self.ship.update(path for path in self.binaries().values() if path.is_file())
        if self.mode == 'coverage' and self.counter.exists():
            self.ship.update(self.counter.rglob('*.gcda'))
        for path in (self.install / 'lib').glob('libtravodds.so*'):
            self.ship.add(path)
        inventory = []
        for path in sorted(self.ship):
            require(path.is_relative_to(ROOT), 'Ship path escapes new evidence root')
            if path.is_symlink():
                target = os.readlink(path)
                require(not Path(target).is_absolute() and '..' not in Path(target).parts,
                        'Ship symlink is not local to its private directory')
                inventory.append({'path': str(path.relative_to(ROOT)), 'symlink': target})
            else:
                require(path.is_file(), 'Missing ship file')
                inventory.append({'path': str(path.relative_to(ROOT)), 'sha256': digest(path),
                                  'bytes': path.stat().st_size})
        manifest_path = self.output / 'ship-manifest.json'
        write(manifest_path, {'schema': 1, 'mode': self.mode, 'files': inventory,
                             'root': str(ROOT), 'no_objects_or_credentials': True})
        self.ship.add(manifest_path)
        archive = ROOT / 'remote' / (self.mode + '.tar.gz')
        require(not archive.exists(), 'Never overwrite a previous remote result archive')
        with tarfile.open(archive, 'w:gz', compresslevel=1) as output:
            for path in sorted(self.ship):
                output.add(path, arcname=str(path.relative_to(ROOT)), recursive=False)
        write(archive.with_suffix(archive.suffix + '.json'), {
            'archive': str(archive), 'sha256': digest(archive), 'bytes': archive.stat().st_size,
            'ship_uncompressed_bytes': sum(row.get('bytes', 0) for row in inventory)
                                       + manifest_path.stat().st_size})
        return archive


def execute(mode, archive, manifest_path, expected_worker_hash, expected_wrapper_hash,
            worker_path, wrapper_path, context=None):
    """One attempt, no retry; return a package even when a command fails."""
    require(mode in MODES, 'Unknown build mode')
    archive, manifest_path = Path(archive), Path(manifest_path)
    require(digest(worker_path) == expected_worker_hash and digest(wrapper_path) == expected_wrapper_hash,
            'Uploaded pipeline helper differs from local frozen helper')
    manifest = json.loads(manifest_path.read_text())
    frozen_source(archive, manifest)
    worker = Worker(mode, manifest, expected_worker_hash, expected_wrapper_hash, context or {})
    shutil.copyfile(manifest_path, worker.output / 'source-manifest.json')
    worker.record['source_manifest_sha256'] = digest(manifest_path)
    worker.record['source_before_sha256'] = hashlib.sha256(canonical(source_map())).hexdigest()
    worker.record['helper_inputs'] = {str(worker_path): digest(worker_path), str(wrapper_path): digest(wrapper_path)}
    worker.save()
    try:
        worker.compiler_fingerprint()
        worker.configure_and_build()
        worker.elf_and_runtimes()
        worker.list_tests()
        try:
            worker.run_payload()
        finally:
            worker.build_provenance()
        require(source_map() == manifest['source_sha256'], 'Source changed during build/test')
        require(digest(archive) == manifest['source_archive_sha256'], 'Source archive changed during worker')
        require(digest(manifest_path) == worker.record['source_manifest_sha256'],
                'Source manifest changed during worker')
        require(digest(worker_path) == expected_worker_hash and digest(wrapper_path) == expected_wrapper_hash,
                'Pipeline helper changed during worker')
        worker.record['source_after_sha256'] = hashlib.sha256(canonical(source_map())).hexdigest()
        worker.finish_shm()
        worker.record['modal_phase_passed'] = True
        worker.record['status'] = 'modal_complete_pending_local_shared_core_native_proof_and_examples'
    except BaseException as error:
        worker.record['error'] = repr(error)
        worker.record['traceback'] = traceback.format_exc()
        worker.record['status'] = 'failed_preserved_no_retry'
        worker.record['modal_phase_passed'] = False
        try:
            worker.record['source_after_sha256'] = hashlib.sha256(canonical(source_map())).hexdigest()
            worker.finish_shm()
        except BaseException as cleanup_error:
            worker.record['cleanup_error'] = repr(cleanup_error)
    finally:
        worker.record['completed_utc'] = utc()
        worker.record['wall_seconds'] = round(time.monotonic() - worker.start, 6)
        worker.record['helper_children_max_rss_kib'] = resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss
        worker.record['acceptance_passed'] = False
        worker.save()
    return worker.package()
