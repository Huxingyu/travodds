#!/usr/bin/env python3
"""Check the upstream unbounded pub/sub examples using actual received payloads.

The runner stops both examples after enough messages, since they have no finite
run option. A runner-requested termination is recorded separately from crashes.
"""
import argparse
import datetime
import json
from pathlib import Path
import re
import subprocess
import time


def run_pair(binary, output, minimum, timeout):
    output.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    processes = {}
    handles = []
    peaks = {"sub": 0, "pub": 0}
    result = {"binary": str(binary), "minimum": minimum, "passed": False}
    try:
        for role in ("sub", "pub"):
            log = (output / f"{role}.log").open("w")
            handles.append(log)
            processes[role] = subprocess.Popen(
                ["stdbuf", "-oL", "-eL", str(binary), role],
                stdout=log, stderr=subprocess.STDOUT, cwd=output,
            )
            if role == "sub":
                time.sleep(1)
        while time.monotonic() - started < timeout:
            for role, process in processes.items():
                if process.poll() is not None:
                    raise RuntimeError(f"{role} exited early: {process.returncode}")
                try:
                    status = Path(f"/proc/{process.pid}/status").read_text()
                    peak = re.search(r"^VmHWM:\s+(\d+)", status, re.M)
                    if peak:
                        peaks[role] = max(peaks[role], int(peak[1]))
                except FileNotFoundError:
                    pass
            sent = re.findall(r"Writing myData, count (\d+)", (output / "pub.log").read_text())
            received = re.findall(
                r"Received data, index (\d+), message ([^\r\n]*)",
                (output / "sub.log").read_text(),
            )
            if len(received) >= minimum:
                indices = [int(index) for index, _ in received]
                if any(message != "hello world" for _, message in received):
                    raise RuntimeError("payload mismatch")
                if any(index not in {int(value) for value in sent} for index in indices):
                    raise RuntimeError("received an index absent from publisher log")
                if any(right <= left for left, right in zip(indices, indices[1:])):
                    raise RuntimeError("duplicate or out-of-order samples")
                result.update(passed=True, received_indices=indices, sent_count=len(sent))
                break
            time.sleep(0.1)
        else:
            raise RuntimeError(f"fewer than {minimum} received samples within {timeout}s")
    except Exception as error:
        result["error"] = str(error)
    finally:
        result["stopped_by_runner"] = []
        for role, process in processes.items():
            if process.poll() is None:
                result["stopped_by_runner"].append(role)
                process.terminate()
        for role, process in processes.items():
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
            result[f"{role}_exit_code"] = process.returncode
        for handle in handles:
            handle.close()
        result.update(elapsed_seconds=round(time.monotonic() - started, 3), peak_rss_kib=peaks)
        (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--minimum", type=int, default=5)
    parser.add_argument("--timeout", type=float, default=25)
    parser.add_argument("--examples", nargs="+",
                        choices=("hello_world", "transmit_shm", "udp_only"),
                        default=("hello_world", "transmit_shm", "udp_only"))
    args = parser.parse_args()
    if args.minimum < 1 or args.timeout <= 1:
        parser.error("minimum must be positive and timeout must exceed one second")
    args.output.mkdir(parents=True, exist_ok=True)
    results = []
    for name in args.examples:
        results.append(run_pair(
            (args.build_root / f"build-{name}" / name).resolve(),
            (args.output / name).resolve(), args.minimum, args.timeout,
        ))
    summary = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "results": results}
    (args.output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))
    return 0 if all(result["passed"] for result in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
