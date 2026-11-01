#!/usr/bin/env python3
"""Validate one downloaded mode on the real kernel; never build or retry.

Importing this file does nothing. Run one --mode at a time after its Modal
archive and receipt are downloaded. Existing artifacts are never overwritten.
"""
from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import resource
import shutil
import signal
import stat
import subprocess
import tarfile
import time
import traceback
import xml.etree.ElementTree as ET


ROOT = Path('/home/ubuntu/project/travodds/artifacts/datasharing-final-20261101')
SOURCE = Path('/home/ubuntu/project/travodds/.worktrees/datasharing-progress')
HERE = Path(__file__).resolve().parent
MODES = ('plain', 'coverage', 'asan', 'tsan')
DIAGNOSTICS = ('WARNING: ThreadSanitizer:', 'ERROR: ThreadSanitizer:',
    'FATAL: ThreadSanitizer:', 'ERROR: AddressSanitizer:', 'AddressSanitizer:DEADLYSIGNAL',
    'runtime error:', 'UndefinedBehaviorSanitizer:')
INSTRUMENTATION = {'plain': (), 'coverage': ('__gcov_',),
                   'asan': ('__asan_', '__ubsan_'), 'tsan': ('__tsan_',)}
SHA256 = re.compile(r'[0-9a-f]{64}')


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=False).encode()


def digest(path):
    result = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            result.update(block)
    return result.hexdigest()


def write_new(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('x') as stream:
        json.dump(value, stream, indent=2, ensure_ascii=False)
        stream.write('\n')


def safe_relative(name):
    path = Path(name)
    require(isinstance(name, str) and name and path.as_posix() == name and
            not path.is_absolute() and '..' not in path.parts and '.' not in path.parts,
            'Unsafe or noncanonical relative path: ' + repr(name))
    return path


def shm():
    result = {}
    for path in sorted(Path('/dev/shm').iterdir()):
        try:
            item = path.lstat()
            result[path.name] = {'inode': item.st_ino, 'device': item.st_dev,
                                 'bytes': item.st_size, 'mode': stat.S_IFMT(item.st_mode)}
        except FileNotFoundError:
            pass
    return result


def processes():
    result = {}
    for path in Path('/proc').iterdir():
        if not path.name.isdigit():
            continue
        try:
            fields = (path / 'stat').read_text().rsplit(')', 1)[1].split()
            result[int(path.name)] = {'state': fields[0], 'parent': int(fields[1]),
                                     'group': int(fields[2]), 'start_ticks': int(fields[19])}
        except (OSError, ValueError, IndexError):
            pass
    return result


def descendants(pid, snapshot):
    result = {pid}
    while True:
        children = {child for child, row in snapshot.items() if row['parent'] in result}
        if children <= result:
            return result
        result.update(children)


def kill_tree(pid, tracked):
    snapshot = processes()
    targets = descendants(pid, snapshot)
    targets.update(child for child, row in snapshot.items() if row['group'] == pid)
    targets.update(child for child, start in tracked.items()
                   if child in snapshot and snapshot[child]['start_ticks'] == start)
    killed = []
    for child in sorted(targets, reverse=True):
        if child == os.getpid():
            continue
        try:
            os.kill(child, signal.SIGKILL)
            killed.append(child)
        except ProcessLookupError:
            pass
    return killed


def listing(text):
    suite, identities = None, []
    for raw in text.splitlines():
        line = raw.split('#', 1)[0].rstrip()
        if re.fullmatch(r'[A-Za-z0-9_./-]+\.', line):
            suite = line[:-1]
        elif suite and re.fullmatch(r'\s+[A-Za-z0-9_./-]+', line):
            identities.append([suite, line.strip()])
    require(identities and len(identities) == len({tuple(row) for row in identities}),
            'Empty or duplicate local gtest listing')
    return sorted(identities)


class LocalStage:
    def __init__(self, mode, archive, receipt, source_manifest, output):
        self.mode = mode
        self.archive, self.receipt_path = Path(archive), Path(receipt)
        self.source_manifest_path = Path(source_manifest)
        self.output = Path(output)
        require(self.output.is_absolute() and self.output.is_relative_to(ROOT / 'local'),
                'Local outputs must live below the new ROOT/local directory')
        self.output.mkdir(parents=True, exist_ok=False)
        self.logs = self.output / 'logs'
        self.logs.mkdir()
        self.labels = set()
        self.remote = self.manifest = None
        self.start = time.monotonic()
        self.counter = ROOT / 'counters/local'
        self.env = dict(os.environ, LC_ALL='C', LANG='C',
                        OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1',
                        CMAKE_BUILD_PARALLEL_LEVEL='1', MAKEFLAGS='-j1')
        for name in list(self.env):
            if name.startswith('GTEST_') or name in ('LD_PRELOAD', 'LD_LIBRARY_PATH',
                    'GCOV_PREFIX', 'GCOV_PREFIX_STRIP'):
                self.env.pop(name, None)
        self.env.update(TSAN_OPTIONS='halt_on_error=1:exitcode=66:second_deadlock_stack=1',
                        ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
                        UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
        # This is fixed before the first TSan process; it is not a retry after
        # a failed run. Root verified setarch is supported on the real host.
        self.instrumented_prefix = ['setarch', 'x86_64', '-R'] if mode == 'tsan' else []
        self.record = {'schema': 1, 'mode': mode, 'started_utc': utc(),
            'local_phase_passed': False, 'combined_mode_passed': False, 'acceptance_passed': False,
            'status': 'verifying_download', 'commands': [], 'xml_results': {},
            'archive': str(self.archive), 'receipt': str(self.receipt_path),
            'source_manifest': str(self.source_manifest_path),
            'helper': str(Path(__file__).resolve()), 'helper_sha256': digest(__file__),
            'resources': {'parallel_programs': 1, 'parallel_build_jobs': 1,
                          'nice': os.getpriority(os.PRIO_PROCESS, 0), 'builds_performed': 0,
                          'test_retries': 0, 'new_swap': False, 'new_spending': False},
            'tsan_execution_policy': {'prefix': self.instrumented_prefix,
                'fixed_before_first_process': True, 'changes_elf_bytes': False,
                'reason': 'Real host known TSan address-layout issue; root preflight confirmed setarch x86_64 -R succeeds. Modal does not use setarch.'
                          if mode == 'tsan' else 'No ASLR wrapper required for this mode.'},
            'sanitizer_options': {key: self.env[key] for key in
                                 ('TSAN_OPTIONS', 'ASAN_OPTIONS', 'UBSAN_OPTIONS')},
            'shm_before': shm(), 'shm_after': None, 'shm_added': None,
            'limitations': ['No local rebuild: downloaded executable/private-library bytes are reused unchanged.',
                'Original remote results remain immutable; local completion does not erase a remote failure.',
                'System glibc/loader are recorded from this real host; portable sanitizer/C++ runtimes are pinned to shipped bytes.',
                'ASan/UBSan disables LSan; this is not a leak scan.',
                'Native payload proof covers software accesses/shared backing, not hardware DRAM write counts.']}
        self.save()

    def save(self):
        # Only this attempt's new mutable status is updated. Raw logs, XML,
        # downloaded files, ship manifest and remote results are never edited.
        temporary = self.output / 'results.json.partial'
        with temporary.open('x') as stream:
            json.dump(self.record, stream, indent=2, ensure_ascii=False)
            stream.write('\n')
        temporary.replace(self.output / 'results.json')

    def run(self, label, argv, timeout=300, instrumented=False, sanitize=False,
            expect_zero=True, cwd=None):
        require(label not in self.labels, 'Duplicate command label: ' + label)
        self.labels.add(label)
        log = self.logs / (label + '.log')
        usage = self.logs / (label + '-resources.txt')
        target = [*self.instrumented_prefix, *map(str, argv)] if instrumented else list(map(str, argv))
        actual = ['nice', '-n', '19', '/usr/bin/time', '-v', '-o', str(usage), *target]
        started_utc, started = utc(), time.monotonic()
        before = shm()
        tracked, timed_out, killed = {}, False, []
        with log.open('xb') as stream:
            process = subprocess.Popen(actual, cwd=cwd or self.output, env=self.env,
                stdin=subprocess.DEVNULL, stdout=stream, stderr=subprocess.STDOUT,
                start_new_session=True)
            while True:
                snapshot = processes()
                for child in descendants(process.pid, snapshot):
                    if child in snapshot:
                        tracked[child] = snapshot[child]['start_ticks']
                left = timeout - (time.monotonic() - started)
                if left <= 0:
                    timed_out = True
                    killed = kill_tree(process.pid, tracked)
                    break
                try:
                    process.wait(timeout=min(0.25, left))
                    break
                except subprocess.TimeoutExpired:
                    # Watchdog only: the command itself is never restarted.
                    continue
            status = process.wait(timeout=15)
            snapshot = processes()
            survivors = {child for child, start in tracked.items() if child != process.pid
                and child in snapshot and snapshot[child]['start_ticks'] == start
                and snapshot[child]['state'] != 'Z'}
            survivors.update(child for child, row in snapshot.items()
                if row['group'] == process.pid and row['state'] != 'Z')
            if survivors:
                killed = sorted(set(killed) | set(kill_tree(process.pid, tracked)))
        text = log.read_text(errors='replace')
        diagnostics = {token: text.count(token) for token in DIAGNOSTICS if token in text}
        after = shm()
        row = {'label': label, 'argv': list(map(str, argv)), 'actual_argv': actual,
            'cwd': str(cwd or self.output), 'started_utc': started_utc,
            'seconds': round(time.monotonic() - started, 6), 'timeout_seconds': timeout,
            'exit_code': status, 'timed_out': timed_out, 'unexpected_survivors': sorted(survivors),
            'killed_pids': killed, 'log': str(log), 'log_sha256': digest(log),
            'resources': str(usage), 'resources_sha256': digest(usage) if usage.exists() else None,
            'sanitizer_diagnostics': diagnostics, 'instrumented_process': instrumented,
            'shm_before': before, 'shm_after': after, 'shm_added': sorted(set(after) - set(before))}
        self.record['commands'].append(row)
        if self.mode == 'tsan' and sanitize and diagnostics:
            self.record['tsan_real_kernel_failure'] = {'label': label, 'diagnostics': diagnostics,
                'known_runtime_mapping_error': 'unexpected memory mapping' in text,
                'stopped_without_retry': True}
        self.save()
        print(json.dumps({'mode': self.mode, 'label': label, 'exit': status,
                          'seconds': row['seconds']}, sort_keys=True), flush=True)
        require(not timed_out and not survivors, 'Timeout or surviving child process: ' + label)
        if sanitize:
            require(not diagnostics, 'Sanitizer diagnostic; stop without retry: ' + label)
        if expect_zero:
            require(status == 0, 'Command failed; stop without retry: ' + label)
        return text, row

    def unpack(self):
        require(self.archive.is_file() and self.receipt_path.is_file(), 'Download or receipt missing')
        receipt = json.loads(self.receipt_path.read_text())
        require(receipt['mode'] == self.mode, 'Download mode mismatch')
        require(self.archive.stat().st_size == receipt['archive_bytes'] and
                digest(self.archive) == receipt['archive_sha256'], 'Downloaded archive SHA/size mismatch')
        require(digest(HERE / 'worker.py') == receipt['worker_sha256'] and
                digest(HERE / 'modal_worker.py') == receipt['modal_wrapper_sha256'],
                'Current pipeline helpers differ from the frozen downloaded attempt')
        self.record['archive_sha256'] = receipt['archive_sha256']
        self.record['receipt_sha256'] = digest(self.receipt_path)
        manifest_name = 'remote/' + self.mode + '/ship-manifest.json'
        allowed = (Path('build') / self.mode, Path('remote') / self.mode, Path('runtime') / self.mode)

        def permitted(path):
            return any(path.is_relative_to(prefix) for prefix in allowed) or (
                self.mode == 'coverage' and path.is_relative_to('counters/modal'))

        extracted, reused = [], []
        with tarfile.open(self.archive, 'r:gz') as archive:
            members = {}
            for member in archive:
                relative = safe_relative(member.name)
                require(permitted(relative), 'Archive path outside this mode: ' + member.name)
                require(member.name not in members, 'Duplicate archive path: ' + member.name)
                require(member.isfile() or member.issym(), 'Archive special/hardlink/directory entry rejected')
                require(not member.mode & 0o7000, 'Archive special permission bits rejected')
                members[member.name] = member
            require(manifest_name in members and members[manifest_name].isfile(), 'No ship manifest')
            stream = archive.extractfile(members[manifest_name])
            require(stream is not None, 'Unreadable ship manifest')
            inventory = json.load(stream)
            require(inventory['mode'] == self.mode and inventory['root'] == str(ROOT),
                    'Ship manifest root/mode mismatch')
            expected = {}
            for entry in inventory['files']:
                relative = safe_relative(entry['path'])
                require(permitted(relative) and entry['path'] not in expected and entry['path'] != manifest_name,
                        'Unsafe or duplicate ship path')
                expected[entry['path']] = entry
                member = members.get(entry['path'])
                require(member is not None, 'Ship entry missing in archive: ' + entry['path'])
                if 'symlink' in entry:
                    link = safe_relative(entry['symlink'])
                    require(member.issym() and member.linkname == str(link), 'Ship symlink mismatch')
                    target = relative.parent / link
                    require(permitted(target), 'Ship symlink escapes allowed tree')
                else:
                    require(member.isfile() and entry['bytes'] == member.size and
                            SHA256.fullmatch(entry['sha256']) is not None, 'Ship file type/size/hash mismatch')
            require(set(members) == set(expected) | {manifest_name}, 'Archive has unlisted members')
            # Validate every existing parent first. No archive member can make
            # a parent-directory symlink, because only files and leaf aliases
            # are permitted and every link target remains in this mode tree.
            for name, member in members.items():
                destination = ROOT / safe_relative(name)
                for parent in destination.parents:
                    require(not parent.is_symlink(), 'Archive extraction parent is symlink: ' + str(parent))
                    if parent == ROOT:
                        break
                destination.parent.mkdir(parents=True, exist_ok=True)
                if destination.exists() or destination.is_symlink():
                    if member.issym():
                        require(destination.is_symlink() and os.readlink(destination) == member.linkname,
                                'Existing file conflicts with shipped symlink: ' + str(destination))
                    else:
                        require(destination.is_file() and not destination.is_symlink(),
                                'Existing destination is not a regular file: ' + str(destination))
                        if name == manifest_name:
                            source = archive.extractfile(member)
                            require(source is not None, 'Cannot recheck existing ship manifest')
                            expected_sha = hashlib.sha256(source.read()).hexdigest()
                        else:
                            expected_sha = expected[name]['sha256']
                        require(destination.stat().st_size == member.size and digest(destination) == expected_sha,
                                'Existing artifact differs; never overwrite: ' + str(destination))
                    reused.append(name)
                    continue
                if member.issym():
                    destination.symlink_to(member.linkname)
                else:
                    source = archive.extractfile(member)
                    require(source is not None, 'Cannot read archive file')
                    with destination.open('xb') as target:
                        shutil.copyfileobj(source, target, length=1024 * 1024)
                    destination.chmod(member.mode & 0o777)
                    if name in expected:
                        require(digest(destination) == expected[name]['sha256'], 'Extracted artifact SHA mismatch')
                extracted.append(name)
        for name, entry in expected.items():
            destination = ROOT / name
            if 'symlink' in entry:
                require(destination.is_symlink() and os.readlink(destination) == entry['symlink'] and
                        destination.resolve().is_relative_to(ROOT) and destination.resolve().is_file(),
                        'Unresolved or escaping shipped alias')
            else:
                require(destination.is_file() and not destination.is_symlink() and
                        digest(destination) == entry['sha256'], 'Final ship inventory SHA mismatch')
        self.inventory = expected
        self.remote_path = ROOT / 'remote' / self.mode / 'results.json'
        self.remote = json.loads(self.remote_path.read_text())
        require(self.remote['mode'] == self.mode and self.remote['source'] == str(SOURCE) and
                self.remote['build'] == str(ROOT / 'build' / self.mode), 'Remote absolute layout mismatch')
        for name in ('worker_sha256', 'modal_wrapper_sha256'):
            require(self.remote[name] == receipt[name], 'Remote helper receipt mismatch: ' + name)
        self.record.update(remote_results=str(self.remote_path), remote_results_sha256=digest(self.remote_path),
            remote_modal_phase_passed=self.remote.get('modal_phase_passed', False),
            remote_original_status=self.remote.get('status'),
            remote_original_error=self.remote.get('error'),
            extracted_members=extracted, reused_identical_members=reused,
            ship_manifest=str(ROOT / manifest_name), ship_manifest_sha256=digest(ROOT / manifest_name),
            ship_manifest_file_count=len(expected))
        self.save()

    def source_identity(self):
        self.manifest = json.loads(self.source_manifest_path.read_text())
        require(digest(ROOT / 'source.tar.gz') == self.manifest['source_archive_sha256'],
                'Frozen source archive changed')
        require(digest(self.source_manifest_path) == self.remote['source_manifest_sha256'],
                'Remote/local source manifest bytes differ')
        expected = self.manifest['source_sha256']
        require(expected and hashlib.sha256(canonical(expected)).hexdigest() ==
                self.manifest['canonical_source_identity_sha256'], 'Noncanonical frozen source map')
        for name in ('implementation_commit', 'canonical_source_identity_sha256', 'source_archive_sha256'):
            require(self.manifest[name] == self.remote[name], 'Remote source identity differs: ' + name)
        require(expected == self.remote['source_sha256'], 'Remote source map differs from frozen map')
        actual = {}
        for name, expected_sha in expected.items():
            path = SOURCE / safe_relative(name)
            require(path.is_file() and not path.is_symlink() and path.resolve().is_relative_to(SOURCE),
                    'Missing or symlinked frozen source: ' + name)
            actual[name] = digest(path)
            require(actual[name] == expected_sha, 'Local source changed: ' + name)
        commit, _ = self.run('source-git-head', ['git', '-C', SOURCE, 'rev-parse', 'HEAD'], timeout=30)
        require(commit.strip() == self.manifest['implementation_commit'], 'Working tree HEAD differs from code identity')
        self.record.update(implementation_commit=self.manifest['implementation_commit'],
            canonical_source_identity_sha256=self.manifest['canonical_source_identity_sha256'],
            source_archive_sha256=self.manifest['source_archive_sha256'],
            source_manifest_sha256=digest(self.source_manifest_path),
            source_before_sha256=hashlib.sha256(canonical(actual)).hexdigest(),
            source_sha256=actual)
        provenance_path = ROOT / 'remote' / self.mode / 'build-provenance.json'
        require(provenance_path.is_file() and digest(provenance_path) == self.remote['build_provenance']['sha256'],
                'Missing or altered compiler provenance')
        provenance = json.loads(provenance_path.read_text())
        compiled_sources = {}
        for name, expected_sha in provenance['compiler_dependency_sha256'].items():
            path = Path(name)
            if path.is_relative_to(SOURCE):
                relative = str(path.relative_to(SOURCE))
                require(relative in expected and expected[relative] == expected_sha,
                        'Compiled input was not in the exact frozen source map: ' + name)
                compiled_sources[name] = expected_sha
        require(compiled_sources, 'No frozen compiler inputs bound to this build')
        for entry in provenance['installed_header_source_aliases']:
            installed, original = Path(entry['installed']), Path(entry['source'])
            require(installed.is_relative_to(ROOT / 'build' / self.mode / 'install/include') and
                    original.is_relative_to(SOURCE / 'include'), 'Invalid installed-header source alias')
            require(digest(installed) == digest(original) == entry['sha256'], 'Installed API header differs')
        for entry in provenance['notes']:
            path = Path(entry['path'])
            require(path.is_relative_to(ROOT / 'build' / self.mode) and
                    digest(path) == entry['sha256'], 'Coverage note differs from compiled ship input')
        self.record['build_provenance'] = {'path': str(provenance_path), 'sha256': digest(provenance_path),
            'frozen_compiler_input_files': len(compiled_sources),
            'installed_header_aliases': len(provenance['installed_header_source_aliases']),
            'notes': len(provenance['notes'])}
        self.save()

    def environment(self):
        expected_dirs = [str(ROOT / 'build' / self.mode / 'install/lib'),
                         str(ROOT / 'runtime' / self.mode / 'portable')]
        require(self.remote['local_ld_library_path'] == expected_dirs, 'Unexpected local loader search directories')
        self.env['LD_LIBRARY_PATH'] = ':'.join(expected_dirs)
        if self.mode == 'coverage':
            require(not self.counter.exists(), 'Local counters already exist; do not merge another attempt implicitly')
            self.counter.mkdir(parents=True)
            self.env.update(GCOV_PREFIX=str(self.counter), GCOV_PREFIX_STRIP='0')
        if self.mode == 'tsan':
            require(platform.machine() == 'x86_64' and shutil.which('setarch'), 'Required fixed local TSan wrapper unavailable')
        self.record['environment'] = {key: self.env.get(key) for key in ('LD_LIBRARY_PATH',
            'GCOV_PREFIX', 'GCOV_PREFIX_STRIP', 'OMP_NUM_THREADS', 'OPENBLAS_NUM_THREADS', 'MKL_NUM_THREADS')}
        kernel, _ = self.run('kernel', ['uname', '-a'], timeout=30)
        self.record['real_kernel'] = kernel.strip()
        self.save()

    def elf(self):
        private = ROOT / 'build' / self.mode / 'install/lib/libtravodds.so.1.1.0'
        require(private.is_file() and not private.is_symlink(), 'Missing shipped private ELF')
        elf_rows = {}
        runtime_rows = self.remote.get('runtime_libraries', [])
        require(runtime_rows, 'No pinned runtime provenance')
        portable_sha = {}
        for entry in runtime_rows:
            copied = Path(entry['copied'])
            require(copied.is_relative_to(ROOT / 'runtime' / self.mode) and digest(copied) == entry['sha256'],
                    'Shipped runtime differs from remote ELF dependency')
            if entry['use_on_ld_library_path']:
                portable_sha[Path(entry['system_alias']).name] = entry['sha256']
        for label, original in self.remote['elf'].items():
            path = Path(original['path'])
            require(path.is_relative_to(ROOT / 'build' / self.mode) and path.is_file() and
                    digest(path) == original['sha256'], 'Shipped ELF changed: ' + label)
            dynamic, _ = self.run('elf-dynamic-' + label, ['readelf', '-d', path], timeout=30)
            notes, _ = self.run('elf-notes-' + label, ['readelf', '-n', path], timeout=30)
            symbols, _ = self.run('elf-symbols-' + label, ['nm', '-D', path], timeout=30)
            loaded, _ = self.run('elf-ldd-' + label, ['ldd', path], timeout=30)
            require('not found' not in loaded, 'Unresolved local dependency: ' + label)
            require(all(token in symbols for token in INSTRUMENTATION[self.mode]),
                    'Missing expected instrumentation: ' + label)
            build_id = re.search(r'Build ID:\s+(\S+)', notes)
            require((build_id.group(1) if build_id else None) == original['build_id'], 'ELF build ID changed')
            if label != 'libtravodds':
                match = re.search(r'libtravodds\.so\S*\s+=>\s+(\S+)', loaded)
                require(match and Path(match.group(1)).resolve() == private.resolve(),
                        'Executable resolved another private library: ' + label)
            dependencies = []
            for line in loaded.splitlines():
                match = re.search(r'=>\s+(/\S+)', line) or re.match(r'\s*(/\S+)', line)
                if not match:
                    continue
                alias = Path(match.group(1))
                resolved = alias.resolve()
                actual_sha = digest(resolved)
                if alias.name in portable_sha:
                    require(resolved.is_relative_to(ROOT / 'runtime' / self.mode / 'portable') and
                            actual_sha == portable_sha[alias.name], 'Portable runtime not resolved to shipped bytes')
                dependencies.append({'path': str(alias), 'resolved': str(resolved), 'sha256': actual_sha,
                    'pinned_portable': alias.name in portable_sha,
                    'same_sha_as_remote_dependency': any(actual_sha == entry['sha256']
                        for entry in original['dependencies'])})
            elf_rows[label] = {'path': str(path), 'sha256': digest(path), 'build_id': original['build_id'],
                'instrumentation_tokens': list(INSTRUMENTATION[self.mode]), 'ldd': loaded,
                'dependencies': dependencies, 'dynamic_section_captured': bool(dynamic)}
        require({'libtravodds', 'payloadpool_test', 'sharedpayloadpool_test'} <= set(elf_rows),
                'Missing required shipped executable identities')
        self.record['elf'] = elf_rows
        self.save()

    def test(self, label, filter_text=None, expected=None, timeout=600):
        info = self.remote['expected_tests'][label]
        binary = Path(info['binary'])
        listed, _ = self.run('list-' + label + ('-native-proof' if filter_text else ''),
            [binary, '--gtest_list_tests'], timeout=90, instrumented=True, sanitize=True)
        require(listing(listed) == info['case_identities'], 'Local list differs from complete remote program: ' + label)
        if expected is None:
            expected = info['case_identities']
        suffix = '-native-proof' if filter_text else ''
        xml_path = self.output / (label + suffix + '.xml')
        command = [binary, '--gtest_output=xml:' + str(xml_path)]
        if filter_text:
            command.append('--gtest_filter=' + filter_text)
        text, row = self.run('test-' + label + suffix, command, timeout=timeout,
            instrumented=True, sanitize=False, expect_zero=False)
        result = {'binary': str(binary), 'binary_sha256': digest(binary), 'filter': filter_text,
            'expected_case_identities': expected, 'expected_count': len(expected),
            'xml': str(xml_path), 'passed': False}
        try:
            root = ET.parse(xml_path).getroot()
            cases = list(root.iter('testcase'))
            identities = sorted([case.get('classname'), case.get('name')] for case in cases)
            result.update(xml_sha256=digest(xml_path), attributes=dict(root.attrib),
                case_identities=identities,
                failures=[{'case': [case.get('classname'), case.get('name')],
                    'failure': [dict(child.attrib, text=child.text) for child in case
                                if child.tag in ('failure', 'error', 'skipped')]}
                    for case in cases if any(child.tag in ('failure', 'error', 'skipped') for child in case)])
            require(identities == sorted(expected) and len(identities) == len({tuple(item) for item in identities}) and
                    len(cases) == int(root.get('tests', '-1')), 'XML omitted, duplicated or added identities')
            require(all(int(root.get(key, '0')) == 0 for key in ('failures', 'errors', 'disabled', 'skipped')),
                    'XML contains failed/disabled/skipped tests')
            require(all(case.get('status') == 'run' and case.get('result') == 'completed' and
                    not any(case.find(key) is not None for key in ('failure', 'error', 'skipped'))
                    for case in cases), 'A test identity did not complete')
            require(row['exit_code'] == 0 and not row['sanitizer_diagnostics'],
                    'Test or sanitizer failed: ' + label + suffix)
            require(not row['shm_added'], 'Test left new shared-memory objects; do not erase: ' + label + suffix)
            result['passed'] = True
        except BaseException as error:
            result['error'] = repr(error)
            raise
        finally:
            self.record['xml_results'][label + suffix] = result
            if self.mode == 'tsan' and row['sanitizer_diagnostics']:
                self.record['tsan_real_kernel_failure'] = {'label': row['label'],
                    'diagnostics': row['sanitizer_diagnostics'],
                    'known_runtime_mapping_error': 'unexpected memory mapping' in text,
                    'stopped_without_retry': True}
            self.save()
        return xml_path, Path(row['log'])

    def assigned_tests(self):
        required = ['sharedpayloadpool_test']
        if self.mode in ('plain', 'coverage'):
            required.append('unit_test')
        for label in required:
            require(self.remote['expected_tests'][label]['assigned_environment'] == 'local-real-kernel',
                    'Local suite assignment was not frozen')
            self.test(label, timeout=1200 if label == 'unit_test' else 600)
        proof = self.remote['local_native_proof']
        expected = [row for row in self.remote['expected_tests']['payloadpool_test']['case_identities']
                    if row[0] == 'NativePayloadWrites']
        require(proof['filter'] == 'NativePayloadWrites.*' and proof['expected_count'] == len(expected) == 4,
                'Native payload witness must contain exactly four frozen identities')
        xml_path, raw_log = self.test('payloadpool_test', proof['filter'], expected, timeout=180)
        audit = self.output / 'native-payload-writes.json'
        self.run('audit-native-payload-writes', ['python3', SOURCE / 'tools/ospp/native_payload_writes.py',
            '--xml', xml_path, '--log', raw_log, '--output', audit], timeout=60)
        summary = json.loads(audit.read_text())
        require(summary['passed'] and summary['tests'] == 4 and not summary['hardware_dram_write_count_measured'],
                'Official raw native proof validation rejected')
        self.record['native_payload_proof'] = {'path': str(audit), 'sha256': digest(audit), 'summary': summary}
        self.save()

    def examples(self):
        if self.mode != 'coverage':
            return
        assignment = self.remote['local_examples']
        require(assignment['rounds'] == 3 and assignment['entrypoint'] == str(SOURCE / 'tools/ospp/datasharing_examples.py'),
                'Example assignment differs from frozen runner')
        folder = self.output / 'examples'
        _, command = self.run('examples-three-rounds', ['python3', assignment['entrypoint'],
            '--build', ROOT / 'build/coverage/examples', '--output', folder,
            '--rounds', '3', '--timeout', '40'], timeout=450, expect_zero=False)
        summary_path = folder / 'summary.json'
        summary = json.loads(summary_path.read_text())
        expected = {(number, name) for number in (1, 2, 3) for name in ('loan', 'keyed', 'mixed')}
        actual = [(row['round'], row['example']) for row in summary['results']]
        records = []
        for row in summary['results']:
            name = row['example']
            require(name in ('loan', 'keyed', 'mixed'), 'Unexpected example identity')
            binary = ROOT / 'build/coverage/examples' / ('datasharing_' + name)
            require(row['binary'] == str(binary) and digest(binary) == self.remote['elf'][binary.name]['sha256'],
                    'Example execution used a different ELF')
            directory = folder / ('round-' + format(row['round'], '03d')) / name
            stdout, stderr = directory / 'stdout.log', directory / 'stderr.log'
            diagnostics = {token: stdout.read_text().count(token) + stderr.read_text().count(token)
                           for token in DIAGNOSTICS}
            records.append({'identity': [row['round'], name], 'argv': [str(binary)],
                'cwd': str(directory), 'binary_sha256': digest(binary), 'exit_code': row.get('exit_code'),
                'writer_pid': row.get('writer_pid'), 'passed': row.get('passed', False),
                'seconds': row.get('elapsed_seconds'), 'stopped_by_runner': row.get('stopped_by_runner'),
                'stdout': str(stdout), 'stdout_sha256': digest(stdout),
                'stderr': str(stderr), 'stderr_sha256': digest(stderr),
                'result_sha256': digest(directory / 'result.json'),
                'sanitizer_diagnostics': {token: count for token, count in diagnostics.items() if count}})
        self.record['examples'] = {'entrypoint_argv': command['argv'], 'summary': str(summary_path),
            'summary_sha256': digest(summary_path), 'commands': records, 'passed': False}
        self.save()
        require(len(actual) == len(set(actual)) == 9 and set(actual) == expected and
                summary['requested_rounds'] == 3 and summary['passed'] and command['exit_code'] == 0,
                'Official examples runner did not complete all nine identities')
        require(all(row['passed'] and row['exit_code'] == 0 and not row['stopped_by_runner'] and
                    not row['sanitizer_diagnostics'] for row in records), 'An example failed or emitted sanitizer output')
        require(not command['shm_added'], 'Examples left shared-memory residue')
        self.record['examples']['passed'] = True
        self.save()

    def finish(self):
        actual = {name: digest(SOURCE / name) for name in self.manifest['source_sha256']}
        require(actual == self.manifest['source_sha256'], 'Frozen source changed during local tests')
        require(digest(self.remote_path) == self.record['remote_results_sha256'], 'Original remote result was modified')
        for name, original in self.remote['elf'].items():
            require(digest(original['path']) == original['sha256'], 'Executed ELF changed during local phase: ' + name)
        for runtime in self.remote['runtime_libraries']:
            require(digest(runtime['copied']) == runtime['sha256'], 'Shipped runtime changed during local phase')
        require(digest(ROOT / 'source.tar.gz') == self.manifest['source_archive_sha256'] and
                digest(self.source_manifest_path) == self.record['source_manifest_sha256'],
                'Frozen source archive/manifest changed during local phase')
        self.record['source_after_sha256'] = hashlib.sha256(canonical(actual)).hexdigest()
        if self.mode == 'coverage':
            counters = [{'path': str(path), 'sha256': digest(path), 'bytes': path.stat().st_size}
                        for path in sorted(self.counter.rglob('*.gcda'))]
            require(counters, 'Local coverage stage produced no separate counters')
            escaped = list((ROOT / 'build/coverage').rglob('*.gcda'))
            require(not escaped, 'Coverage counters escaped the dedicated local GCOV_PREFIX')
            self.record['raw_coverage_counters'] = counters
            self.record['coverage_counter_policy'] = {'local_prefix': str(self.counter),
                'modal_prefix_preserved': str(ROOT / 'counters/modal'), 'merged_here': False,
                'raw_notes_and_counters_not_rewritten': True}
        self.record['local_phase_passed'] = True
        self.record['combined_mode_passed'] = bool(self.remote.get('modal_phase_passed'))
        self.record['status'] = 'local_complete_remote_also_passed' if self.record['combined_mode_passed'] else (
            'local_complete_remote_failed_requires_diagnosis')
        self.save()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=MODES, required=True)
    parser.add_argument('--archive', type=Path)
    parser.add_argument('--receipt', type=Path)
    parser.add_argument('--source-manifest', type=Path, default=ROOT / 'source-manifest.json')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    os.setpriority(os.PRIO_PROCESS, 0, 19)
    require(os.getpriority(os.PRIO_PROCESS, 0) == 19, 'Local phase must run at nice 19')
    archive = args.archive or ROOT / 'downloads' / (args.mode + '.tar.gz')
    receipt = args.receipt or archive.with_suffix(archive.suffix + '.json')
    stage = LocalStage(args.mode, archive.resolve(), receipt.resolve(), args.source_manifest.resolve(),
                       (args.output or ROOT / 'local' / args.mode).resolve())
    try:
        stage.unpack()
        stage.source_identity()
        stage.environment()
        stage.elf()
        stage.record['status'] = 'running_assigned_real_kernel_tests'
        stage.save()
        stage.assigned_tests()
        stage.examples()
        stage.finish()
    except BaseException as error:
        stage.record.update(error=repr(error), traceback=traceback.format_exc(),
                            status='failed_preserved_no_retry', local_phase_passed=False,
                            combined_mode_passed=False)
    finally:
        after = shm()
        added = sorted(set(after) - set(stage.record['shm_before']))
        stage.record.update(completed_utc=utc(), wall_seconds=round(time.monotonic() - stage.start, 6),
            helper_children_max_rss_kib=resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss,
            shm_after=after, shm_added=added, shm_cleanup_performed=False)
        if added:
            stage.record.update(status='failed_new_shm_preserved', local_phase_passed=False,
                                combined_mode_passed=False)
        stage.save()
    print(json.dumps({'mode': args.mode, 'local_phase_passed': stage.record['local_phase_passed'],
        'combined_mode_passed': stage.record['combined_mode_passed'],
        'results': str(stage.output / 'results.json'), 'shm_added': stage.record['shm_added']}, indent=2), flush=True)
    return 0 if stage.record['combined_mode_passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
