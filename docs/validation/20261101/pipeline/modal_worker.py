"""One explicitly requested Modal attempt; no work runs on import.

After the final source commit is frozen, run one mode at a time:
  nice -n 19 modal run pipeline/modal_worker.py --mode plain
  nice -n 19 modal run pipeline/modal_worker.py --mode coverage
  nice -n 19 modal run pipeline/modal_worker.py --mode asan
  nice -n 19 modal run pipeline/modal_worker.py --mode tsan

Each invocation exclusively creates downloads/<mode>.tar.gz. A returned archive
is evidence of an attempt, never a claim that local validation has completed.
"""
from pathlib import Path
import hashlib
import importlib.util
import json
import os
import shutil
import subprocess
import modal


if modal.is_local():
    HERE = Path(__file__).resolve().parent
    ROOT = HERE.parent
    WORKER_SHA256 = hashlib.sha256((HERE / 'worker.py').read_bytes()).hexdigest()
    WRAPPER_SHA256 = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    image = (modal.Image.from_registry('ubuntu:24.04', add_python='3.12')
        .apt_install('g++', 'cmake', 'make', 'unzip', 'util-linux', 'time', 'git', 'binutils')
        .add_local_file(str(ROOT / 'source.tar.gz'), '/opt/travodds-source.tar.gz')
        .add_local_file(str(ROOT / 'source-manifest.json'), '/opt/travodds-source-manifest.json')
        .add_local_file(str(HERE / 'worker.py'), '/opt/travodds-final-worker.py')
        .add_local_file(str(Path(__file__)), '/opt/travodds-final-modal-worker.py'))
else:
    # Modal imports this entrypoint by filename under /root. That import must
    # not try to reopen local sibling files or rebuild local upload mounts.
    ROOT = Path('/home/ubuntu/project/travodds/artifacts/datasharing-final-20261101')
    image = modal.Image.from_registry('ubuntu:24.04', add_python='3.12')
app = modal.App('travodds-final-20261101', image=image)


@app.function(cpu=2.0, memory=4096, timeout=900, retries=0)
def build(mode: str, expected_worker_sha: str, expected_wrapper_sha: str,
          expected_manifest_sha: str, expected_archive_sha: str):
    if mode not in ('plain', 'coverage', 'asan', 'tsan'):
        raise ValueError('Unknown mode')
    spec = importlib.util.spec_from_file_location('travodds_final_worker', '/opt/travodds-final-worker.py')
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    if helper.digest('/opt/travodds-source-manifest.json') != expected_manifest_sha:
        raise ValueError('Uploaded source manifest differs from the local frozen manifest')
    if helper.digest('/opt/travodds-source.tar.gz') != expected_archive_sha:
        raise ValueError('Uploaded source archive differs from the local frozen archive')
    context = {'image_base': 'ubuntu:24.04', 'python_requested': '3.12',
               'modal_sdk_version': getattr(modal, '__version__', None)}
    call_id = getattr(modal, 'current_function_call_id', None)
    if call_id is not None:
        context['function_call_id'] = call_id()
    archive = helper.execute(mode, '/opt/travodds-source.tar.gz',
        '/opt/travodds-source-manifest.json', expected_worker_sha, expected_wrapper_sha,
        '/opt/travodds-final-worker.py', '/opt/travodds-final-modal-worker.py', context)
    payload = archive.read_bytes()
    archive_info = json.loads(archive.with_suffix(archive.suffix + '.json').read_text())
    return {'archive': payload, 'archive_sha256': hashlib.sha256(payload).hexdigest(),
            'archive_bytes': len(payload), 'ship_uncompressed_bytes': archive_info['ship_uncompressed_bytes'],
            'mode': mode}


@app.local_entrypoint()
def main(mode: str):
    if mode not in ('plain', 'coverage', 'asan', 'tsan'):
        raise ValueError('Choose exactly one of plain, coverage, asan, tsan')
    destination = ROOT / 'downloads' / (mode + '.tar.gz')
    receipt = destination.with_suffix(destination.suffix + '.json')
    if destination.exists() or receipt.exists():
        raise FileExistsError('Use a new attempt directory; existing results are never overwritten')
    if os.getpriority(os.PRIO_PROCESS, 0) != 19:
        raise RuntimeError('Launch the local CLI with nice -n 19')
    if shutil.disk_usage(ROOT).free < 768 * 1024 ** 2:
        raise RuntimeError('Need at least 768 MiB free before a paid remote attempt and download')
    manifest_path = ROOT / 'source-manifest.json'
    manifest = json.loads(manifest_path.read_text())
    expected_manifest_sha = hashlib.sha256(manifest_path.read_bytes()).hexdigest()
    expected_archive_sha = hashlib.sha256((ROOT / 'source.tar.gz').read_bytes()).hexdigest()
    if expected_archive_sha != manifest['source_archive_sha256']:
        raise ValueError('Local frozen source archive SHA mismatch')
    source_identity = hashlib.sha256(json.dumps(manifest['source_sha256'], sort_keys=True,
        separators=(',', ':'), ensure_ascii=False).encode()).hexdigest()
    if source_identity != manifest['canonical_source_identity_sha256']:
        raise ValueError('Local source manifest canonical identity mismatch')
    source = Path('/home/ubuntu/project/travodds/.worktrees/datasharing-progress')
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=source, text=True).strip()
    if head != manifest['implementation_commit']:
        raise ValueError('Local HEAD changed after source freeze')
    for name, expected in manifest['source_sha256'].items():
        path = source / name
        if path.is_symlink() or hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            raise ValueError('Local frozen input changed: ' + name)
    result = build.remote(mode, WORKER_SHA256, WRAPPER_SHA256,
                          expected_manifest_sha, expected_archive_sha)
    payload = result.pop('archive')
    if len(payload) != result['archive_bytes'] or hashlib.sha256(payload).hexdigest() != result['archive_sha256']:
        raise ValueError('Downloaded result integrity mismatch')
    if shutil.disk_usage(ROOT).free < len(payload) + 500 * 1024 ** 2:
        raise RuntimeError('Result received but saving it would leave less than 500 MiB free')
    destination.parent.mkdir(parents=True, exist_ok=True)
    with destination.open('xb') as output:
        output.write(payload)
    with receipt.open('x') as output:
        json.dump({**result, 'path': str(destination), 'worker_sha256': WORKER_SHA256,
                   'modal_wrapper_sha256': WRAPPER_SHA256,
                   'source_manifest_sha256': expected_manifest_sha,
                   'source_archive_sha256': expected_archive_sha,
                   'local_nice': os.getpriority(os.PRIO_PROCESS, 0)}, output, indent=2)
        output.write('\n')
    print(json.dumps({'attempt_saved': str(destination), **result}, indent=2))
