#!/usr/bin/env python3
"""Read-only final verification; no tests, builds, Modal calls or gcov runs.

Run only when four fresh remote/local modes, coverage, the deterministic
diagnosis, app-stop listing and cleanup evidence are complete. Successful
outputs audit/final-audit.json and audit/inputs-manifest.json are exclusive.
Missing, failed or inconsistent evidence produces no success artifact.
"""
from __future__ import annotations

import datetime
import gzip
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tarfile
import xml.etree.ElementTree as ET
import zipfile


ROOT = Path(__file__).resolve().parent.parent
SOURCE = Path('/home/ubuntu/project/travodds/.worktrees/datasharing-progress')
PROTECTED = tuple(ROOT.parent / name for name in
                  ('strict-ring-20261031', 'portable-evidence-20261031'))
MODES = ('plain', 'coverage', 'asan', 'tsan')
TOKENS = ('WARNING: ThreadSanitizer:', 'ERROR: ThreadSanitizer:',
    'FATAL: ThreadSanitizer:', 'ERROR: AddressSanitizer:',
    'AddressSanitizer:DEADLYSIGNAL', 'runtime error:', 'UndefinedBehaviorSanitizer:')
INSTRUMENTATION = {'plain': (), 'coverage': ('__gcov_',),
                   'asan': ('__asan_', '__ubsan_'), 'tsan': ('__tsan_',)}
BASELINE = '489067108405b27afbd369245542f5e88d457f18'
INPUTS, CACHE = {}, {}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=False).encode()


def sha(path):
    path = Path(path).absolute()
    info = path.stat()
    key = (str(path), info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns, info.st_ctime_ns)
    if key not in CACHE:
        digest = hashlib.sha256()
        with path.open('rb') as stream:
            for block in iter(lambda: stream.read(1048576), b''):
                digest.update(block)
        CACHE[key] = digest.hexdigest()
    result = CACHE[key]
    require(INPUTS.setdefault(str(path), result) == result, 'Input changed during audit: ' + str(path))
    return result


def checked(path, expected):
    require(sha(path) == expected, 'SHA mismatch: ' + str(path))


def load(path):
    sha(path)
    return json.loads(Path(path).read_text())


def command(argv, env=None):
    return subprocess.check_output(list(map(str, argv)), cwd=SOURCE, env=env,
                                   text=True, stderr=subprocess.STDOUT)


def module(path, name):
    sha(path)
    spec = importlib.util.spec_from_file_location(name, path)
    helper = importlib.util.module_from_spec(spec)
    previous = sys.dont_write_bytecode
    sys.dont_write_bytecode = True
    try:
        spec.loader.exec_module(helper)
    finally:
        sys.dont_write_bytecode = previous
    return helper


def identities(path, expected=None, passed=True):
    sha(path)
    root = ET.parse(path).getroot()
    cases = list(root.iter('testcase'))
    result = sorted([case.get('classname'), case.get('name')] for case in cases)
    require(cases and all(all(item) for item in result) and
            len(result) == len({tuple(item) for item in result}) == int(root.get('tests', '-1')),
            'Missing, duplicate or inconsistent XML identities: ' + str(path))
    if expected is not None:
        require(result == sorted(expected), 'XML/listing identity mismatch: ' + str(path))
    if passed:
        for suite in [root, *root.iter('testsuite')]:
            require(all(int(suite.get(key, '0')) == 0 for key in
                        ('failures', 'errors', 'disabled', 'skipped')), 'Failed or incomplete suite: ' + str(path))
        require(all(case.get('status') == 'run' and case.get('result') == 'completed' and
                    not any(case.find(tag) is not None for tag in ('failure', 'error', 'skipped'))
                    for case in cases), 'Failed/incomplete XML case: ' + str(path))
    return result, dict(root.attrib), cases


def log(path, clean=False):
    sha(path)
    text = Path(path).read_text(errors='strict')
    if clean:
        require(not any(token in text for token in TOKENS), 'Sanitizer diagnostic: ' + str(path))
    return text


def listing(text):
    current, result = None, []
    for row in text.splitlines():
        row = row.split('#', 1)[0].rstrip()
        if re.fullmatch(r'[A-Za-z0-9_./-]+\.', row):
            current = row[:-1]
        elif current and re.fullmatch(r'\s+[A-Za-z0-9_./-]+', row):
            result.append([current, row.strip()])
    require(result and len(result) == len({tuple(item) for item in result}), 'Invalid raw test listing')
    return sorted(result)


def commands(record, local=False):
    rows = record['commands']
    require(rows and len(rows) == len({row['label'] for row in rows}), 'Empty or duplicate command records')
    result = {}
    for row in rows:
        require(row['exit_code'] == 0 and not row.get('timed_out') and
                not row.get('unexpected_survivors'), 'Failed command: ' + row['label'])
        checked(row['log'], row['log_sha256'])
        log(row['log'], clean=True)
        if row.get('resources_sha256'):
            checked(row['resources'], row['resources_sha256'])
        require(not row.get('sanitizer_diagnostics'), 'Recorded sanitizer diagnostic')
        if local:
            require(row['actual_argv'][:3] == ['nice', '-n', '19'], 'Local command not run at nice 19')
            prefix = ['setarch', 'x86_64', '-R'] if record['mode'] == 'tsan' and row['instrumented_process'] else []
            require(row['actual_argv'] == ['nice', '-n', '19', '/usr/bin/time', '-v', '-o', row['resources'],
                                           *prefix, *row['argv']], 'Local executed argv differs from recorded command')
            require(row['shm_added'] == sorted(set(row['shm_after']) - set(row['shm_before'])) == [],
                    'Local command left new SHM: ' + row['label'])
        else:
            require(row['actual_argv'] == ['/usr/bin/time', '-v', '-o', row['resources'], *row['argv']],
                    'Remote executed argv differs from recorded command')
        result[row['label']] = row
    return result


def protected():
    before = load(ROOT / 'protected-before.json')
    actual = {}
    for folder in PROTECTED:
        for path in folder.rglob('*'):
            info = path.lstat()
            actual[str(path)] = [info.st_mode, info.st_size, info.st_mtime_ns, info.st_ino]
    require(actual == before, 'Protected directory path set or lstat metadata changed')
    return len(before)


def freeze():
    saved = load(ROOT / 'source-manifest.json')
    source_map = saved['source_sha256']
    require(len(source_map) == saved['input_count'] and source_map, 'Source manifest count mismatch')
    identity = hashlib.sha256(canonical(source_map)).hexdigest()
    require(identity == saved['canonical_source_identity_sha256'], 'Noncanonical source identity')
    tracked = command(['git', 'ls-files', '-z', '--', *saved['archive_paths']]).split('\0')
    require(set(filter(None, tracked)) == set(source_map), 'Frozen input inventory differs from selected tracked source')
    require(not command(['git', 'status', '--porcelain', '--', *saved['archive_paths']]).strip(),
            'Selected build source has pending tracked/untracked changes')
    checked(ROOT / 'source.tar.gz', saved['source_archive_sha256'])
    archived = {}
    with tarfile.open(ROOT / 'source.tar.gz', 'r:gz') as archive:
        for member in archive:
            if member.isdir():
                continue
            path = Path(member.name)
            require(member.isfile() and not path.is_absolute() and '..' not in path.parts
                    and member.name not in archived, 'Unsafe/duplicate frozen source archive entry')
            stream = archive.extractfile(member)
            require(stream is not None, 'Unreadable frozen source')
            digest = hashlib.sha256()
            for block in iter(lambda: stream.read(1048576), b''):
                digest.update(block)
            archived[member.name] = digest.hexdigest()
    require(archived == source_map, 'Frozen archive differs from manifest')
    request = ''.join(saved['implementation_commit'] + ':' + name + '\n' for name in source_map).encode()
    data = subprocess.check_output(['git', 'cat-file', '--batch'], cwd=SOURCE, input=request)
    offset = 0
    for name, expected in source_map.items():
        checked(SOURCE / name, expected)
        require(not (SOURCE / name).is_symlink(), 'Frozen source became a symlink')
        end = data.index(b'\n', offset)
        header = data[offset:end].split()
        require(len(header) == 3 and header[1] == b'blob', 'Missing committed frozen input: ' + name)
        length, offset = int(header[2]), end + 1
        require(hashlib.sha256(data[offset:offset + length]).hexdigest() == expected,
                'Frozen input differs from claimed code commit: ' + name)
        offset += length + 1
    require(offset == len(data), 'Unexpected git batch response')
    # Documentation may subsequently advance HEAD; source bytes still have to
    # match the final code commit. Code ancestry is checked independently.
    command(['git', 'merge-base', '--is-ancestor', saved['implementation_commit'], 'HEAD'])
    return saved


def ship(mode, remote):
    receipt_path = ROOT / 'downloads' / (mode + '.tar.gz.json')
    receipt = load(receipt_path)
    archive = ROOT / 'downloads' / (mode + '.tar.gz')
    checked(archive, receipt['archive_sha256'])
    require(archive.stat().st_size == receipt['archive_bytes'] and receipt['mode'] == mode,
            'Download receipt mismatch')
    for field, script in (('worker_sha256', 'worker.py'), ('modal_wrapper_sha256', 'modal_worker.py')):
        require(receipt[field] == remote[field] == sha(ROOT / 'pipeline' / script),
                'Executed pipeline helper differs from frozen helper: ' + script)
    manifest_path = ROOT / 'remote' / mode / 'ship-manifest.json'
    inventory = load(manifest_path)
    require(inventory['mode'] == mode and inventory['root'] == str(ROOT), 'Ship root/mode mismatch')
    names = set()
    for row in inventory['files']:
        name = Path(row['path'])
        require(not name.is_absolute() and '..' not in name.parts and row['path'] not in names,
                'Invalid ship inventory path')
        names.add(row['path'])
        path = ROOT / name
        if 'symlink' in row:
            require(path.is_symlink() and os.readlink(path) == row['symlink'] and
                    path.resolve().is_relative_to(ROOT), 'Ship alias changed')
        else:
            checked(path, row['sha256'])
            require(path.stat().st_size == row['bytes'], 'Ship file size changed')
    with tarfile.open(archive, 'r:gz') as handle:
        members = handle.getmembers()
        require(len(members) == len(names) + 1 and {member.name for member in members} ==
                names | {str(manifest_path.relative_to(ROOT))}, 'Archive/member inventory mismatch')
    return {'archive_sha256': receipt['archive_sha256'], 'files': len(names)}


def raw_native(record, helper, expected_xml, expected_log):
    saved = load(record['path'])
    checked(record['path'], record['sha256'])
    require(saved == record['summary'] and saved['passed'] is True and saved['tests'] == 4,
            'Native proof summary mismatch')
    require(not saved['hardware_dram_write_count_measured'], 'Invalid hardware proof claim')
    require(saved['inputs']['xml']['path'] == str(expected_xml) and
            saved['inputs']['log']['path'] == str(expected_log) and
            saved['inputs']['auditor']['path'] == str(SOURCE / 'tools/ospp/native_payload_writes.py') and
            saved['inputs']['test_source']['path'] == str(SOURCE / 'test/unit_payloadpool/test_native_payload_writes.cpp'),
            'Native proof does not bind the actual current-mode test outputs/helper/source')
    for value in saved['inputs'].values():
        checked(value['path'], value['sha256'])
        require(Path(value['path']).stat().st_size == value['bytes'], 'Native proof input size changed')
    completed = helper.xml_cases(Path(saved['inputs']['xml']['path']))
    records = helper.log_records(Path(saved['inputs']['log']['path']))
    actual = []
    for name in helper.CASES:
        case = helper.validate_record(name, records[name])
        case['test_result'] = completed[name]
        actual.append(case)
    require(actual == saved['cases'], 'Native proof does not recompute from raw evidence')
    return {'tests': 4, 'raw_revalidated': True, 'sha256': record['sha256'],
            'fault_signals': [row['fault_signal'] for row in actual]}


def elf(mode, remote, local):
    build = ROOT / 'build' / mode
    library = build / 'install/lib/libtravodds.so.1.1.0'
    paths = {'libtravodds': library, 'payloadpool_test': build / 'tests/payloadpool_test',
             'sharedpayloadpool_test': build / 'tests/sharedpayloadpool_test'}
    if mode in ('plain', 'coverage'):
        paths['unit_test'] = build / 'core/unit_test'
    if mode == 'coverage':
        paths.update({name: build / 'examples' / name for name in
                      ('datasharing_loan', 'datasharing_keyed', 'datasharing_mixed')})
    require(set(remote['elf']) == set(local['elf']) == set(paths), 'Remote/local/required ELF inventories differ')
    env = dict(os.environ, LD_LIBRARY_PATH=':'.join(remote['local_ld_library_path']))
    env.pop('LD_PRELOAD', None)
    portable = {}
    for row in remote['runtime_libraries']:
        checked(row['copied'], row['sha256'])
        require(Path(row['copied_alias']).resolve() == Path(row['copied']).resolve(), 'Runtime alias changed')
        if row['use_on_ld_library_path']:
            portable[Path(row['system_alias']).name] = row['sha256']
    results = {}
    for name, original in remote['elf'].items():
        current = local['elf'][name]
        path = Path(original['path'])
        require(path == paths[name] and current['path'] == str(path), 'ELF uses an unexpected mode/target path')
        checked(path, original['sha256'])
        require(current['sha256'] == original['sha256'] and current['build_id'] == original['build_id'],
                'Local execution used a changed ELF')
        symbols = command(['nm', '-D', path])
        require(all(token in symbols for token in INSTRUMENTATION[mode]), 'ELF lacks expected instrumentation')
        require('travodds_statistics_diagnostic_locked' not in symbols,
                'Diagnostic-only hook leaked into a formal ELF')
        notes = command(['readelf', '-n', path])
        build_id = re.search(r'Build ID:\s+(\S+)', notes)
        require(build_id and build_id.group(1) == original['build_id'], 'ELF build ID does not recompute')
        loaded = command(['ldd', path], env=env)
        require('not found' not in loaded, 'Unresolved current ELF dependency')
        for text in (original['ldd'], current['ldd'], loaded):
            if name != 'libtravodds':
                match = re.search(r'libtravodds\.so\S*\s+=>\s+(\S+)', text)
                require(match and Path(match.group(1)).resolve() == library.resolve(),
                        'An executable resolved a different DDS library')
        for dependency in current['dependencies']:
            checked(dependency['resolved'], dependency['sha256'])
            filename = Path(dependency['path']).name
            if filename in portable:
                require(dependency['pinned_portable'] and dependency['sha256'] == portable[filename],
                        'Local sanitizer/C++ runtime does not use shipped bytes')
        results[name] = {'sha256': original['sha256'], 'build_id': original['build_id'],
                         'private_library_verified': True, 'instrumentation': list(INSTRUMENTATION[mode])}
    return results


def provenance(mode, remote, source_map, common_external):
    path = ROOT / 'remote' / mode / 'build-provenance.json'
    checked(path, remote['build_provenance']['sha256'])
    saved = load(path)
    dependency_names = set()
    recorded_paths = set()
    for row in saved['records']:
        checked(row['path'], row['sha256'])
        require(row['path'] not in recorded_paths, 'Duplicate build provenance record')
        recorded_paths.add(row['path'])
        if row['path'].endswith('.o.d'):
            text = Path(row['path']).read_text().replace('\\\n', ' ')
            require(':' in text, 'Malformed compiler dependency evidence')
            for token in shlex.split(text.split(':', 1)[1]):
                dependency = Path(token)
                require(dependency.is_absolute(), 'Unmapped relative compiler dependency')
                dependency_names.add(str(dependency.resolve()))
    require(dependency_names == set(saved['compiler_dependency_sha256']),
            'Recorded compiler-input map differs from actual dependency files')
    for row in saved['notes']:
        checked(row['path'], row['sha256'])
    require(not saved['api_header_resolution']['installed_include_dependencies'] and
            saved['api_header_resolution']['source_include_sha256'], 'API include canonicalization failed')
    zip_path = SOURCE / 'test/googletest-1.17.0.zip'
    sha(zip_path)
    with zipfile.ZipFile(zip_path) as archive:
        zip_names = archive.namelist()
        for name, expected in saved['compiler_dependency_sha256'].items():
            dependency = Path(name)
            if dependency.is_relative_to(SOURCE):
                relative = str(dependency.relative_to(SOURCE))
                require(source_map.get(relative) == expected, 'Compiled repository input was not frozen')
                checked(dependency, expected)
            elif 'googletest-src' in dependency.parts:
                index = dependency.parts.index('googletest-src')
                suffix = '/'.join(dependency.parts[index + 1:])
                matches = [name for name in zip_names if name == suffix or name.endswith('/' + suffix)]
                require(len(matches) == 1 and hashlib.sha256(archive.read(matches[0])).hexdigest() == expected,
                        'Extracted GTest compiler input differs from frozen ZIP')
            elif not dependency.is_relative_to(ROOT / 'build' / mode):
                require(common_external.setdefault(name, expected) == expected,
                        'System compiler dependency differs across fresh modes')
    for row in saved['installed_header_source_aliases']:
        checked(row['installed'], row['sha256'])
        checked(row['source'], row['sha256'])
    build = ROOT / 'build' / mode
    links = [build / 'library/src/CMakeFiles/travodds.dir/link.txt',
             build / 'tests/CMakeFiles/payloadpool_test.dir/link.txt',
             build / 'tests/CMakeFiles/sharedpayloadpool_test.dir/link.txt']
    if mode in ('plain', 'coverage'):
        links.append(build / 'core/CMakeFiles/unit_test.dir/link.txt')
    if mode == 'coverage':
        links += [build / 'examples/CMakeFiles' / (name + '.dir/link.txt') for name in
                  ('datasharing_loan', 'datasharing_keyed', 'datasharing_mixed', 'datasharing_demo')]
    linked_notes = set()
    object_count = 0
    for link in links:
        sha(link)
        require(str(link) in recorded_paths, 'A linked target lacks build provenance')
        flags = link.parent / 'flags.make'
        sha(flags)
        require(str(flags) in recorded_paths, 'A linked target lacks compiler flags')
        match = re.search(r'^CXX_FLAGS = (.*)$', flags.read_text(), re.MULTILINE)
        require(match is not None, 'A linked target has no C++ flags')
        options = shlex.split(match.group(1))
        required_options = {'-I' + str(SOURCE / 'include')}
        if mode == 'coverage':
            required_options |= {'--coverage', '-fprofile-update=atomic'}
        elif mode in ('asan', 'tsan'):
            required_options.add('-fsanitize=' + ('address,undefined' if mode == 'asan' else 'thread'))
        require(required_options <= set(options), 'Actual compiler flags lack required instrumentation or API priority')
        cwd = link.parent.parent.parent
        target_objects = 0
        for token in shlex.split(link.read_text()):
            if not token.endswith('.o'):
                continue
            obj = (cwd / token).resolve() if not Path(token).is_absolute() else Path(token)
            require(obj.is_relative_to(build), 'Linked object came from a reused/outside build')
            dependency_file = Path(str(obj) + '.d')
            sha(dependency_file)
            require(str(dependency_file) in recorded_paths, 'A linked object lacks recorded compiler inputs')
            object_count += 1
            target_objects += 1
            if mode == 'coverage':
                linked_notes.add(str(obj.with_suffix('.gcno')))
        require(target_objects, 'A required linked target contains no fresh objects')
    actual_notes = {row['path'] for row in saved['notes'] if not row['official_collector_excluded_dependency']}
    require(mode != 'coverage' or linked_notes == actual_notes,
            'Coverage notes do not exactly cover all linked production/test/example objects')
    return {'linked_objects': object_count, 'notes': len(saved['notes']),
            'official_notes': len(actual_notes), 'compiler_dependency_files':
            len(saved['compiler_dependency_sha256']), 'path': str(path)}


def one_mode(mode, manifest, proof, common_external):
    remote_path, local_path = ROOT / 'remote' / mode / 'results.json', ROOT / 'local' / mode / 'results.json'
    remote, local = load(remote_path), load(local_path)
    require(remote['modal_phase_passed'] is True and local['local_phase_passed'] is True and
            local['combined_mode_passed'] is True, 'Mode has not completed successfully: ' + mode)
    require(remote['acceptance_passed'] is False and local['acceptance_passed'] is False,
            'A partial runner prematurely claimed complete acceptance')
    for record in (remote, local):
        require(record['mode'] == mode and record['source_sha256'] == manifest['source_sha256'], 'Mode source map differs')
        for key in ('implementation_commit', 'canonical_source_identity_sha256', 'source_archive_sha256'):
            require(record[key] == manifest[key], 'Mode source binding differs: ' + key)
        require(record['source_manifest_sha256'] == sha(ROOT / 'source-manifest.json') and
                record['source_before_sha256'] == record['source_after_sha256'] ==
                manifest['canonical_source_identity_sha256'], 'Source was not stable through validation')
        require(record['shm_added'] == sorted(set(record['shm_after']) - set(record['shm_before'])) == [],
                'Mode left new SHM')
    require(remote['resources']['cpu'] == 2 and remote['resources']['memory_mib'] == 4096
            and remote['resources']['parallel_build_jobs'] == 2 and remote['resources']['retries'] == 0,
            'Remote allocation/retry policy differs')
    require(local['resources']['nice'] == 19 and local['resources']['builds_performed'] == 0 and
            local['resources']['test_retries'] == 0, 'Local compute/retry policy differs')
    checked(local['helper'], local['helper_sha256'])
    checked(local['remote_results'], local['remote_results_sha256'])
    inventory = ship(mode, remote)
    remote_commands, local_commands = commands(remote), commands(local, local=True)
    expected = remote['expected_tests']
    required = {'payloadpool_test', 'sharedpayloadpool_test'} | ({'unit_test'} if mode in ('plain', 'coverage') else set())
    require(set(expected) == required, 'Unexpected aggregate program inventory')
    for label, info in expected.items():
        require(info['binary'] == remote['elf'][label]['path'] and
                remote_commands['list-' + label]['argv'] == [info['binary'], '--gtest_list_tests'],
                'Remote listing is not bound to the required ELF')
        require(info['case_identities'] == listing(log(remote_commands['list-' + label]['log'], True))
                and len(info['case_identities']) == info['count'], 'Expected identities differ from raw listing')
    actual, attributes, _ = identities(ROOT / 'remote' / mode / 'payloadpool_test.xml',
                                      expected['payloadpool_test']['case_identities'])
    require(attributes == remote['payloadpool_result']['attributes'], 'Remote XML summary changed')
    checked(ROOT / 'remote' / mode / 'payloadpool_test.xml', remote['payloadpool_result']['sha256'])
    require(remote_commands['payloadpool_test']['argv'] == [expected['payloadpool_test']['binary'],
            '--gtest_output=xml:' + str(ROOT / 'remote' / mode / 'payloadpool_test.xml')],
            'Modal aggregate execution uses the wrong ELF, filter or XML path')
    all_ids = {tuple(item) for item in actual}
    for label in sorted(required - {'payloadpool_test'}):
        require(expected[label]['assigned_environment'] == 'local-real-kernel', 'Missing complete local assignment')
        result = local['xml_results'][label]
        require(result['passed'] is True and not result['filter'], 'Local aggregate was filtered or failed')
        binary = expected[label]['binary']
        require(result['binary'] == binary and result['binary_sha256'] == remote['elf'][label]['sha256'] and
                result['xml'] == str(ROOT / 'local' / mode / (label + '.xml')) and
                local_commands['list-' + label]['argv'] == [binary, '--gtest_list_tests'] and
                local_commands['test-' + label]['argv'] == [binary, '--gtest_output=xml:' + result['xml']],
                'Local aggregate execution is not bound to the required ELF/output')
        require(listing(log(local_commands['list-' + label]['log'], True)) == expected[label]['case_identities'],
                'Local raw listing differs from the actual complete remote program')
        actual, attrs, _ = identities(result['xml'], expected[label]['case_identities'])
        checked(result['xml'], result['xml_sha256'])
        require(actual == result['case_identities'] and attrs == result['attributes'], 'Local XML result mismatch')
        require(not all_ids.intersection(map(tuple, actual)), 'Aggregate identities overlap')
        all_ids.update(map(tuple, actual))
    native_ids = [item for item in expected['payloadpool_test']['case_identities'] if item[0] == 'NativePayloadWrites']
    local_native = local['xml_results']['payloadpool_test-native-proof']
    native_binary = expected['payloadpool_test']['binary']
    require(local_native['binary'] == native_binary and
            local_native['binary_sha256'] == remote['elf']['payloadpool_test']['sha256'] and
            local_native['xml'] == str(ROOT / 'local' / mode / 'payloadpool_test-native-proof.xml') and
            local_commands['list-payloadpool_test-native-proof']['argv'] == [native_binary, '--gtest_list_tests'] and
            local_commands['test-payloadpool_test-native-proof']['argv'] == [native_binary,
                '--gtest_output=xml:' + local_native['xml'], '--gtest_filter=NativePayloadWrites.*'],
            'Local native proof execution uses an unexpected ELF, filter or XML path')
    require(listing(log(local_commands['list-payloadpool_test-native-proof']['log'], True)) ==
            expected['payloadpool_test']['case_identities'], 'Local native binary has an incomplete raw listing')
    identities(local_native['xml'], native_ids)
    require(local_native['passed'] is True and local_native['filter'] == 'NativePayloadWrites.*'
            and len(native_ids) == 4, 'Local native proof identity mismatch')
    checked(local_native['xml'], local_native['xml_sha256'])
    required_builds = {'configure-library', 'build-library', 'configure-modules', 'build-modules'}
    if mode in ('plain', 'coverage'):
        required_builds |= {'configure-core', 'build-core'}
    if mode == 'coverage':
        required_builds |= {'configure-examples', 'build-examples'}
    require(required_builds <= set(remote_commands), 'Fresh build command is missing')
    require(remote_commands['build-modules']['argv'][-3:] == ['--target', 'payloadpool_test', 'sharedpayloadpool_test'],
            'Duplicate standalone targets were built instead of the specified aggregate scope')
    for key in ('ASAN_OPTIONS', 'TSAN_OPTIONS', 'UBSAN_OPTIONS'):
        require(local['sanitizer_options'][key] == remote['environment_policy']['recorded'][key],
                'Sanitizer policy differs across environments')
    result = {'passed': True, 'unique_tests': len(all_ids),
        'payloadpool_tests': expected['payloadpool_test']['count'],
        'sharedpool_tests': expected['sharedpayloadpool_test']['count'],
        'case_identity_sha256': hashlib.sha256(canonical(sorted(map(list, all_ids)))).hexdigest(),
        'ship': inventory, 'elf': elf(mode, remote, local),
        'provenance': provenance(mode, remote, manifest['source_sha256'], common_external),
        'native_payload_proof': {'modal': raw_native(remote['native_payload_proof'], proof,
            ROOT / 'remote' / mode / 'payloadpool_test.xml', remote_commands['payloadpool_test']['log']),
            'local': raw_native(local['native_payload_proof'], proof, local_native['xml'],
                local_commands['test-payloadpool_test-native-proof']['log'])}}
    if 'unit_test' in expected:
        result['core_tests'] = expected['unit_test']['count']
    return result, remote, local


def examples(local):
    info = local['examples']
    saved = load(info['summary'])
    checked(info['summary'], info['summary_sha256'])
    require(info['passed'] is True and saved['passed'] is True and saved['requested_rounds'] == 3,
            'Examples did not finish three rounds')
    expected = {(number, name) for number in (1, 2, 3) for name in ('loan', 'keyed', 'mixed')}
    require(len(saved['results']) == len(info['commands']) == 9 and
            {(row['round'], row['example']) for row in saved['results']} == expected,
            'Missing or duplicate example identity')
    for result, row in zip(saved['results'], info['commands']):
        require(row['identity'] == [result['round'], result['example']] and result['passed'] is True and
                result['exit_code'] == 0 and not result['stopped_by_runner'], 'Example status mismatch')
        checked(row['stdout'], row['stdout_sha256'])
        checked(row['stderr'], row['stderr_sha256'])
        log(row['stderr'], clean=True)
        records = [json.loads(line[len('DS_RESULT '):]) for line in log(row['stdout'], True).splitlines()
                   if line.startswith('DS_RESULT ')]
        require(records == result['records'] and len(records) == 2 and
                {item['role'] for item in records} == {'writer', 'reader'}, 'Example raw records mismatch')
        writer, reader = (next(item for item in records if item['role'] == role) for role in ('writer', 'reader'))
        require(writer['pid'] == result['writer_pid'] and writer['pid'] != reader['pid'], 'Example process identity mismatch')
        name = result['example']
        samples = 5 if name == 'keyed' else 6
        for item in records:
            is_reader = item['role'] == 'reader'
            values = {'example': name, 'samples': samples, 'native_loans': samples if is_reader else 0,
                'controls': 2 if is_reader and name == 'keyed' else 0,
                'serializations': 6 if not is_reader and name == 'mixed' else 0,
                'deserializations': 6 if is_reader and name == 'mixed' else 0,
                'held_after_deletes': is_reader}
            require(all(item.get(key) == value for key, value in values.items()), 'Example semantics differ')
        directory = Path(reader['directory'])
        require(reader['directory'] == writer['directory'] and directory.parent == Path('/dev/shm')
                and directory.name.startswith('travodds-demo-') and not directory.exists(), 'Example SHM remains')
        result_path = Path(row['cwd']) / 'result.json'
        checked(result_path, row['result_sha256'])
        require({**load(result_path), 'round': result['round']} == result, 'Per-example result mismatch')
        checked(result['binary'], row['binary_sha256'])
        expected_binary = str(ROOT / 'build/coverage/examples' / ('datasharing_' + name))
        require(result['binary'] == expected_binary and row['argv'] == [expected_binary] and
                local['elf']['datasharing_' + name]['path'] == expected_binary and
                row['binary_sha256'] == local['elf']['datasharing_' + name]['sha256'],
                'Example execution is not bound to the current coverage ELF')
    return {'passed': True, 'runs': 9, 'raw_records_revalidated': True}


def coverage(helper, remote, local, manifest):
    folder = ROOT / 'coverage'
    record = load(folder / 'results.json')
    require(record['passed'] is True and record['baseline'] == BASELINE and record['raw_counters_unchanged'],
            'Official coverage stage did not pass')
    checked(SOURCE / 'tools/ospp/coverage.py', record['official_collector_sha256'])
    checked(ROOT / 'pipeline/coverage_stage.py', record['helper_sha256'])
    require(record['source_manifest_sha256'] == sha(ROOT / 'source-manifest.json') and
            record['implementation_commit'] == manifest['implementation_commit'] and
            record['canonical_source_identity_sha256'] == record['source_before_sha256'] ==
            record['source_after_sha256'] == manifest['canonical_source_identity_sha256'],
            'Coverage source identity differs from the final frozen build')
    require(remote['environment_policy']['coverage'] ==
            {'GCOV_PREFIX': str(ROOT / 'counters/modal'), 'GCOV_PREFIX_STRIP': '0'} and
            local['coverage_counter_policy']['local_prefix'] == str(ROOT / 'counters/local'),
            'Coverage did not use distinct fresh counter prefixes')
    for row in record['commands']:
        require(row['exit_code'] == 0, 'Coverage command failed')
        checked(folder / (row['label'] + '.log'), row['log_sha256'])
    prefixes = {str(ROOT / 'counters/modal'), str(ROOT / 'counters/local')}
    require(set(record['raw_counter_sha256']) == prefixes, 'Coverage raw counter prefixes differ')
    for prefix, expected in record['raw_counter_sha256'].items():
        actual = {str(path): sha(path) for path in sorted(Path(prefix).rglob('*.gcda'))}
        require(actual == expected and actual, 'Raw counter inventory changed')
    for stage, environment in ((remote, 'modal'), (local, 'local')):
        prefix = ROOT / 'counters' / environment
        rows = stage['raw_coverage_counters']
        stage_map = {row['path']: row['sha256'] for row in rows}
        require(len(stage_map) == len(rows) and stage_map == record['raw_counter_sha256'][str(prefix)],
                'Coverage stage omitted, duplicated or substituted runtime counters')
        for row in stage['raw_coverage_counters']:
            require(Path(row['path']).is_relative_to(prefix) and Path(row['path']).stat().st_size == row['bytes'],
                    'Coverage runtime counter path/size differs from its producing stage')
            checked(row['path'], row['sha256'])
    require(not list((ROOT / 'build/coverage').rglob('*.gcda')), 'Runtime counters escaped dedicated prefixes')
    notes = record['notes']
    provenance_saved = load(ROOT / 'remote/coverage/build-provenance.json')
    require({row['original']: row['sha256'] for row in notes} ==
            {row['path']: row['sha256'] for row in provenance_saved['notes']}, 'Coverage omitted original notes')
    official_notes = {row['copy'] for row in notes if not row['official_excluded']}
    require(len(notes) == record['notes_count'] and len(official_notes) == record['official_notes_count'],
            'Coverage note counts differ')
    for row in notes:
        checked(row['original'], row['sha256'])
        checked(row['copy'], row['sha256'])
        require(Path(row['copy']).with_suffix('.gcda').is_file() == bool(row['counter']),
                'Merged note/data presence mismatch')
        if row['counter']:
            checked(row['counter'], row['counter_sha256'])
            checked(Path(row['copy']).with_suffix('.gcda'), row['counter_sha256'])
    saved = load(folder / 'report/summary.json')
    require(saved == record['summary'] and saved['baseline'] == BASELINE, 'Official coverage summary mismatch')
    changed = helper.added_lines(SOURCE, BASELINE)
    counts = {name: {} for name in changed}
    reports, negatives = [], []
    for path in sorted((folder / 'report').rglob('*.gcov.json.gz')):
        sha(path)
        with gzip.open(path, 'rt') as stream:
            report = json.load(stream)
        reports.append(report['data_file'])
        cwd = Path(report.get('current_working_directory', SOURCE))
        for file in report['files']:
            file_path = Path(file['file'])
            if not file_path.is_absolute():
                file_path = cwd / file_path
            try:
                name = str(file_path.resolve().relative_to(SOURCE))
            except ValueError:
                continue
            for row in file['lines']:
                require(type(row['count']) is int, 'Invalid gcov count type')
                if name.startswith(('src/', 'include/')) and row['count'] < 0:
                    negatives.append({'file': name, 'line': row['line_number'], 'count': row['count']})
                if name in counts:
                    line = row['line_number']
                    counts[name][line] = counts[name].get(line, 0) + row['count']
    require(len(reports) == len(set(reports)) == len(official_notes) and set(reports) == official_notes,
            'Raw gcov reports omit zero-data translation units or duplicate notes')
    missing = [name for name in changed if name.endswith('.cpp') and not counts[name]]
    require(not missing, 'Changed production translation units missing from coverage')
    details = {}
    for name, lines in sorted(changed.items()):
        executable = sorted(lines.intersection(counts[name]))
        uncovered = [line for line in executable if counts[name][line] <= 0]
        details[name] = {'added_lines': len(lines), 'executable_added_lines': len(executable),
            'covered_added_lines': len(executable) - len(uncovered), 'uncovered_added_lines': uncovered,
            'source_sha256': sha(SOURCE / name)}
    total = sum(row['executable_added_lines'] for row in details.values())
    covered = sum(row['covered_added_lines'] for row in details.values())
    require(total and saved['files'] == details and saved['executable_added_lines'] == total and
            saved['covered_added_lines'] == covered and saved['percent'] == round(100 * covered / total, 3)
            and saved['translation_units'] == len(official_notes) and saved['passed'] is True
            and 100 * covered / total >= saved['required_percent'] == 70, 'Coverage failed independent recomputation')
    return {'passed': True, 'translation_units': len(official_notes), 'covered_added_lines': covered,
        'executable_added_lines': total, 'percent': saved['percent'], 'required_percent': 70,
        'negative_production_counts': len(negatives), 'negative_count_policy': 'Counts <=0 are uncovered; no clipping or absolute-value conversion.',
        'baseline': BASELINE, 'raw_json_recomputed_without_running_gcov': True}


def statistics(manifest):
    folder = ROOT / 'statistics/attempt2/statistics-result'
    result = load(folder / 'results.json')
    require(result['passed'] is True and not result['shm_added'], 'Statistics diagnosis did not finish')
    for name, expected in result['input_sha256'].items():
        path = (ROOT / 'statistics/baseline-source-v2.tar.gz' if name == 'baseline-source.tar.gz'
                else folder / 'inputs' / name)
        checked(path, expected)
    original = load(folder / 'inputs/baseline-inputs.json')
    archive = load(folder / 'inputs/baseline-source.json')
    require(original['head'] == archive['commit'] == result['source_commit'] and
            original['sha256'] == result['source_sha256'] and
            archive['archive_sha256'] == result['input_sha256']['baseline-source.tar.gz'],
            'Diagnostic baseline/source-archive identity mismatch')
    for row in result['commands']:
        require(row['exit_code'] == 0, 'A diagnostic preparation/runner command failed')
        checked(folder / (row['label'] + '.log'), row['log_sha256'])
    prepared = load(folder / 'prepared/manifest.json')
    require(sha(folder / 'prepared/manifest.json') == result['prepared_manifest_sha256'], 'Diagnostic manifest changed')
    reviewed = load(folder / 'inputs/reviewed-generated-manifest.json')
    require({key: value for key, value in prepared.items() if key not in ('observed_head', 'source_root')} ==
            {key: value for key, value in reviewed.items() if key not in ('observed_head', 'source_root')},
            'Remote diagnostic differs from the independently reviewed preparation')
    require(prepared['baseline'] == original and prepared['sources'] == original['sha256'] and
            prepared['reviewed_head'] == result['source_commit'], 'Prepared diagnostic baseline mismatch')
    for name, expected in prepared['tools'].items():
        checked(folder / 'inputs' / name, expected)
    for name, value in prepared['generated'].items():
        checked(folder / 'prepared' / name, value)
    candidate = folder / 'prepared/test_loan-after-candidate.cpp'
    require(sha(candidate) == manifest['source_sha256']['test/unit_payloadpool/test_loan.cpp'],
            'Final loan test differs from the proved candidate')
    for name, expected_code in (('before', 1), ('after', 0)):
        stage = folder / name
        saved = load(stage / 'summary.json')
        require(saved['diagnostic_witness_valid'] is True and saved['returncode'] == expected_code
                and not saved['timed_out'] and not saved['process_group_survivors'] and not saved['shm_added'],
                'Invalid deterministic ' + name + ' witness')
        require(saved['checks'] and all(saved['checks'].values()), 'A diagnostic check failed')
        require(saved['binary_sha256'] == result['binaries'][name]['sha256'] and
                saved['manifest_sha256'] == result['prepared_manifest_sha256'] and
                saved['source_sha256'] == result['source_sha256'] and
                set(saved['library_sha256'].values()) == {result['library_sha256']},
                'Diagnostic runner used different inputs')
        require(result['binaries'][name]['generated_source_sha256'] ==
                prepared['generated']['loan-' + name + '.cpp'], 'Diagnostic ELF source binding differs')
        before_shm, after_shm = load(stage / 'shm-before.json'), load(stage / 'shm-after.json')
        require(not (set(after_shm) - set(before_shm)), 'Diagnostic raw SHM snapshot contains new residue')
        raw_events = [json.loads(line[len('TRAVODDS_STATS_DIAG '):]) for line in log(stage / 'test.log').splitlines()
                      if line.startswith('TRAVODDS_STATS_DIAG ')]
        require(raw_events == load(stage / 'events.json'), 'Diagnostic raw event stream differs')
        names = [row['event'] for row in raw_events]
        holders = [row for row in raw_events if row['event'] == 'CHILD_HOOK_LOCKED']
        require(len(holders) == 1 and holders[0]['mutex_owner_tid'] == holders[0]['tid']
                and holders[0]['pid'] != holders[0]['tid'] and
                holders[0]['parent_pid'] == saved['parent_process_pid'] != holders[0]['pid'] and
                'PARENT_CONFIRMED_CHILD_LOCK' in names,
                'Actual listener thread holding the mutex was not witnessed')
        confirmations = [row for row in raw_events if row['event'] == 'PARENT_CONFIRMED_CHILD_LOCK']
        require(len(confirmations) == 1 and confirmations[0]['pid'] == saved['parent_process_pid'] and
                confirmations[0]['holder_pid'] == holders[0]['pid'] and
                confirmations[0]['holder_tid'] == holders[0]['tid'] and
                confirmations[0]['mutex'] == holders[0]['mutex'],
                'Parent did not confirm the actual child mutex owner')
        frames = load(stage / 'backtrace-resolved.json')
        raw_frames = [row for row in raw_events if row['event'] == 'CHILD_HOOK_BACKTRACE_FRAME']
        require([row['frame'] for row in frames] == raw_frames, 'Resolved stack differs from raw holder frames')
        witnesses = [row for row in frames if row['returncode'] == 0 and
                     'readerdatasharing.cpp' in row['stdout']]
        require(witnesses,
                'Actual holder stack does not resolve to the native listener')
        for witness in witnesses:
            require(Path(witness['frame']['object']).name.startswith('libtravodds.so') and
                    command(['addr2line', '-f', '-C', '-i', '-e', folder / 'private-lib/libtravodds.so.1.1.0',
                             witness['frame']['offset']]) == witness['stdout'],
                    'Actual listener stack does not independently resolve against the retained diagnostic ELF')
        _, _, cases = identities(stage / 'result.xml', passed=(name == 'after'))
        require(len(cases) == 1 and cases[0].get('classname') + '.' + cases[0].get('name') == saved['target'],
                'Diagnostic executed a different target')
        if name == 'before':
            failures = [child for case in cases for child in case if child.tag in ('failure', 'error')]
            require(len(failures) == 1 and 'pool->statistics(closed)' in failures[0].get('message', '') and
                    re.search(r'(?:<02-00\s+00-00>|TIMEOUT)', failures[0].get('message', '')),
                    'Before witness is not the required statistics TIMEOUT failure')
            reaped = [row for row in raw_events if row['event'] == 'BEFORE_FAILURE_CLEANUP_REAPED']
            require(len(reaped) == 1 and reaped[0]['child_wait_status'] == 0,
                    'Expected RED witness did not reap its child cleanly')
        else:
            order = ['CHILD_HOOK_LOCKED', 'CHILD_BEGIN_DELETE_READER', 'PARENT_RELEASE_AFTER_J',
                'CHILD_HOOK_RESUMED', 'CHILD_DELETE_READER_RETURNED_WITH_LOAN',
                'PARENT_RECEIVED_C_JOIN_CONFIRMED', 'PARENT_ORIGINAL_STATISTICS_OK', 'CHILD_COMPLETED_PROTOCOL']
            require(all(names.count(item) == 1 for item in order) and
                    [names.index(item) for item in order] == sorted(names.index(item) for item in order),
                    'After witness did not prove listener join before original exact statistics assertions')
        checked(folder / ('loan-' + name), result['binaries'][name]['sha256'])
    checked(folder / 'private-lib/libtravodds.so.1.1.0', result['library_sha256'])
    attempt1 = load(ROOT / 'statistics/attempt1/statistics-result/results.json')
    require(attempt1['passed'] is False, 'Historical setup failure was changed into a pass')
    return {'passed': True, 'evidence': str(folder), 'before_returncode': 1, 'after_returncode': 0,
        'historical_holder_unproved': True,
        'historical_setup_failure_preserved': True,
        'interpretation': 'A controlled real listener interleaving proves a valid source of TIMEOUT; the historical failing run did not record its holder.'}


def cleanup(plan):
    saved = load(ROOT / 'audit/cleanup.json')
    require(saved['passed'] is True and not saved['shm_added'], 'Cleanup record did not pass')
    count = protected()
    baseline = set(plan['shm_before'])
    current = {path.name for path in Path('/dev/shm').iterdir()}
    require(not (current - baseline), 'Final host has new shared-memory entries')
    rows = load(ROOT / 'audit/modal-app-list.json')
    require(isinstance(rows, list), 'Expected raw Modal app-list JSON array')
    selected = [row for row in rows if row.get('description') in
                ('travodds-final-20261101', 'travodds-final-statistics')]
    require({row['description'] for row in selected} ==
            {'travodds-final-20261101', 'travodds-final-statistics'} and
            len(selected) == len({row['app_id'] for row in selected}),
            'Final Modal apps missing/duplicated in stop evidence')
    require(all(str(row.get('state', '')).lower() == 'stopped' and int(row.get('tasks', '-1')) == 0
                for row in selected), 'A task Modal app remains active')
    return {'passed': True, 'shm_added': [], 'protected_paths': count, 'modal_apps_stopped': True,
            'modal_app_rows': selected,
            'protected_scope': 'Exact path-set and lstat mode/size/mtime_ns/inode; no original all-file content-hash inventory exists.'}


def budget(plan, remote):
    saved = load(ROOT / 'usage-ledger.json')
    entries = saved['entries']
    require(saved['not_invoice'] is True and saved['budget_usd'] == plan['budget_usd'] == 1 and entries and
            len(entries) == len({row['label'] for row in entries}) and all(row['settled'] for row in entries),
            'Budget authorization/settled-attempt inventory mismatch')
    indexed = {row['label']: row for row in entries}
    require(set(indexed) == set(MODES) | {'statistics', 'statistics-v2'},
            'Paid-attempt ledger is missing an actual invocation or has unbound entries')
    for mode in MODES:
        receipt = load(ROOT / 'downloads' / (mode + '.tar.gz.json'))
        require(indexed[mode]['downloaded_bytes'] == receipt['archive_bytes'] and
                remote[mode]['wall_seconds'] <= indexed[mode]['observed_seconds'],
                'Budget undercounts a formal remote runtime or download')
    for label, attempt, filename in (
            ('statistics', 'attempt1', 'modal-diagnosis.tar.gz'),
            ('statistics-v2', 'attempt2', 'modal-diagnosis-v2.tar.gz')):
        result = load(ROOT / 'statistics' / attempt / 'statistics-result/results.json')
        archive = ROOT / 'statistics' / filename
        sha(archive)
        require(indexed[label]['downloaded_bytes'] == archive.stat().st_size and
                indexed[label]['observed_seconds'] >= result['elapsed_seconds'],
                'Budget undercounts a diagnostic attempt')
    require(all(0 <= row['observed_seconds'] <= row['reserved_seconds'] and
                type(row['downloaded_bytes']) is int and row['downloaded_bytes'] >= 0 for row in entries),
            'Invalid settled runtime/download amount')
    rate = plan['modal_cpu'] * plan['function_cpu_usd_per_core_second'] + \
           plan['modal_memory_mib'] / 1024 * plan['function_memory_usd_per_gib_second']
    require(abs(rate - plan['requested_allocation_rate_usd_per_second']) < 1e-12,
            'Requested allocation pricing arithmetic differs')
    seconds = sum(row['observed_seconds'] for row in entries)
    transfer = sum(row['downloaded_bytes'] for row in entries) / 2**30 * plan['network_egress_usd_per_gib']
    requested = round(seconds * rate + transfer, 6)
    conservative = round(seconds * plan['provisional_conservative_rate_usd_per_s'] + transfer +
                         plan['image_and_transfer_reserve_usd'], 6)
    require(abs(seconds - saved['total_observed_or_reserved_seconds']) < 1e-9 and
            requested == saved['requested_allocation_plus_egress_usd'] and
            conservative == saved['conservative_including_reserve_usd'] <= saved['budget_usd'],
            'Budget totals do not recompute within USD1')
    sha(ROOT / plan['pricing_snapshot'])
    return {name: saved[name] for name in ('budget_usd', 'conservative_including_reserve_usd',
            'requested_allocation_plus_egress_usd', 'not_invoice')}


def main():
    outputs = (ROOT / 'audit/final-audit.json', ROOT / 'audit/inputs-manifest.json')
    require(not any(path.exists() for path in outputs), 'Never overwrite successful final audit artifacts')
    require(os.getpriority(os.PRIO_PROCESS, 0) == 19, 'Run this read-only audit with nice -n 19')
    self_hash = sha(__file__)
    manifest = freeze()
    plan = load(ROOT / 'resource-plan.json')
    require(plan['budget_usd'] == 1 and plan['modal_cpu'] == 2 and plan['modal_memory_mib'] == 4096
            and plan['local_jobs'] == 1 and plan['local_nice'] == 19 and not plan['new_swap'], 'Resource policy differs')
    proof = module(SOURCE / 'tools/ospp/native_payload_writes.py', 'final_native_proof_helper')
    official = module(SOURCE / 'tools/ospp/coverage.py', 'final_official_coverage_helper')
    external, modes, remote, local = {}, {}, {}, {}
    for mode in MODES:
        modes[mode], remote[mode], local[mode] = one_mode(mode, manifest, proof, external)
    reference = remote['plain']['expected_tests']
    for mode in MODES:
        for label, row in remote[mode]['expected_tests'].items():
            require(row['case_identities'] == reference[label]['case_identities'],
                    'Fresh modes disagree on program identities')
    result = {'schema': 1, 'passed': True, 'implementation_commit': manifest['implementation_commit'],
        'audited_head': command(['git', 'rev-parse', 'HEAD']).strip(),
        'canonical_source_identity_sha256': manifest['canonical_source_identity_sha256'],
        'source_input_count': len(manifest['source_sha256']), 'auditor_sha256': self_hash,
        'modes': modes, 'examples': examples(local['coverage']),
        'coverage': coverage(official, remote['coverage'], local['coverage'], manifest),
        'statistics': statistics(manifest), 'cleanup': cleanup(plan)}
    result['budget'] = budget(plan, remote)
    result['pipeline_differences'] = [
        'Four completely fresh builds replace historical object reuse; counts derive from raw listings/link provenance, not 324/144 fixed cardinalities.',
        'Modal runs the full payload aggregate; all shared-pool/core identities run unchanged ELF bytes on the real kernel. Raw environments remain separate.',
        'Source and build absolute paths agree across environments; source -I precedes imported system headers. Raw gcov JSON paths are never rewritten.',
        'Official atomic coverage uses preserved two-environment counters; audit independently recomputes existing raw JSON without invoking gcov.',
        'Every mode binds runtime libraries, ELF hashes/build IDs, complete identities and both raw native proofs; statistics RED remains an expected failure witness.',
    ]
    result['limitations'] = [
        'Sanitizers cover module/shared-pool integration; core tests run in plain and coverage modes. LSan is disabled.',
        'System compiler dependency hashes are compared across fresh Modal images; shipped portable runtimes are pinned. Host glibc/loader identities are recorded separately.',
        'Return-code-3 historical cause remains unproved; performance and cross-host fallback limitations remain as documented.',
        'Native software payload guards do not measure hardware DRAM writes; lock-free scope is limited to notification/shared data plane.',
    ]
    require(sha(__file__) == self_hash and protected() == result['cleanup']['protected_paths'], 'Auditor/protected tree changed')
    for name, expected in list(INPUTS.items()):
        checked(name, expected)
    for name, expected in manifest['source_sha256'].items():
        checked(SOURCE / name, expected)
    require(not ({path.name for path in Path('/dev/shm').iterdir()} - set(plan['shm_before'])), 'SHM appeared during final audit')
    inputs = {'schema': 1, 'implementation_commit': manifest['implementation_commit'],
        'canonical_source_identity_sha256': manifest['canonical_source_identity_sha256'],
        'auditor_sha256': self_hash, 'file_sha256': dict(sorted(INPUTS.items())),
        'common_modal_system_dependency_sha256': dict(sorted(external.items()))}
    input_bytes = json.dumps(inputs, indent=2, ensure_ascii=False).encode() + b'\n'
    result['inputs_manifest_sha256'] = hashlib.sha256(input_bytes).hexdigest()
    result['completed_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    result_bytes = json.dumps(result, indent=2, ensure_ascii=False).encode() + b'\n'
    outputs[0].parent.mkdir(parents=True, exist_ok=True)
    with outputs[1].open('xb') as output:
        output.write(input_bytes)
    with outputs[0].open('xb') as output:
        output.write(result_bytes)
    print(json.dumps({'passed': True, 'implementation_commit': result['implementation_commit'],
        'tests': {mode: row['unique_tests'] for mode, row in modes.items()},
        'coverage_percent': result['coverage']['percent'], 'outputs': list(map(str, outputs))}, indent=2))


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, RuntimeError, KeyError, AssertionError, ET.ParseError,
            subprocess.CalledProcessError) as error:
        print('Final audit rejected: ' + str(error), file=sys.stderr)
        raise SystemExit(1)
