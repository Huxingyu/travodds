#!/usr/bin/env python3
"""Run the three finite public-API DataSharing examples, sequentially."""
import argparse
import datetime
import json
import os
from pathlib import Path
import signal
import subprocess
import time


def stop_group(pid):
    try:
        os.killpg(pid, signal.SIGKILL)
    except ProcessLookupError:
        pass


def run(binary, output, name, timeout):
    output.mkdir(parents=True, exist_ok=False)
    started = time.monotonic()
    result = {"example": name, "binary": str(binary), "passed": False,
              "stopped_by_runner": False}
    process = None
    try:
        with (output / "stdout.log").open("w") as stdout, (output / "stderr.log").open("w") as stderr:
            process = subprocess.Popen([str(binary)], cwd=output, stdout=stdout,
                                       stderr=stderr, start_new_session=True)
            result["writer_pid"] = process.pid
            try:
                result["exit_code"] = process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                result["stopped_by_runner"] = True
                stop_group(process.pid)
                result["exit_code"] = process.wait()
                raise RuntimeError("Example did not exit naturally within deadline")
        if result["exit_code"] != 0:
            raise RuntimeError(f"Example exited with {result['exit_code']}")
        # A successful launcher must also reap its subscriber, not leave it running.
        try:
            os.killpg(process.pid, 0)
        except ProcessLookupError:
            pass
        else:
            result["stopped_by_runner"] = True
            stop_group(process.pid)
            raise RuntimeError("Example left processes in its private process group")
        records = [json.loads(line.removeprefix("DS_RESULT "))
                   for line in (output / "stdout.log").read_text().splitlines()
                   if line.startswith("DS_RESULT ")]
        result["records"] = records
        if len(records) != 2 or {r["role"] for r in records} != {"reader", "writer"}:
            raise RuntimeError("Expected exactly one reader and one writer result")
        by_role = {r["role"]: r for r in records}
        if by_role["writer"]["pid"] != process.pid or by_role["reader"]["pid"] == process.pid:
            raise RuntimeError("Expected independent writer and reader processes")
        expected_samples = 5 if name == "keyed" else 6
        directories = set()
        for r in records:
            reader = r["role"] == "reader"
            expected = {"example": name, "samples": expected_samples,
                        "native_loans": expected_samples if reader else 0,
                        "controls": 2 if reader and name == "keyed" else 0,
                        "serializations": 6 if not reader and name == "mixed" else 0,
                        "deserializations": 6 if reader and name == "mixed" else 0,
                        "held_after_deletes": reader}
            for key, value in expected.items():
                if r.get(key) != value:
                    raise RuntimeError(f"{r['role']} {key}: {r.get(key)!r}, expected {value!r}")
            directory = Path(r["directory"])
            if directory.parent != Path("/dev/shm") or not directory.name.startswith("travodds-demo-"):
                raise RuntimeError("Unexpected shared directory")
            if directory.exists():
                raise RuntimeError(f"Shared directory was not removed: {directory}")
            directories.add(str(directory))
        if len(directories) != 1:
            raise RuntimeError("Writer and reader used different shared directories")
        diagnostics = (output / "stderr.log").read_text()
        if "ERROR: AddressSanitizer" in diagnostics or "runtime error:" in diagnostics:
            raise RuntimeError("Sanitizer reported a diagnostic")
        result["passed"] = True
    except Exception as error:
        result["error"] = str(error)
    finally:
        if process is not None and not result["passed"]:
            stop_group(process.pid)
            process.wait()
        result["elapsed_seconds"] = round(time.monotonic() - started, 3)
        (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True,
                        help="CMake build directory containing datasharing_* executables")
    parser.add_argument("--output", type=Path, required=True,
                        help="Fresh result directory; existing results are never overwritten")
    parser.add_argument("--rounds", type=int, default=1)
    parser.add_argument("--timeout", type=float, default=40)
    args = parser.parse_args()
    if args.rounds < 1 or args.timeout <= 0:
        parser.error("rounds and timeout must be positive")
    args.output.mkdir(parents=True, exist_ok=False)
    results = []
    for round_number in range(1, args.rounds + 1):
        for name in ("loan", "keyed", "mixed"):
            result = run((args.build / f"datasharing_{name}").resolve(),
                         (args.output / f"round-{round_number:03d}" / name).resolve(),
                         name, args.timeout)
            result["round"] = round_number
            results.append(result)
            print(f"round {round_number} {name}: {'PASS' if result['passed'] else result.get('error')}", flush=True)
            if not result["passed"]:
                break
        if not results[-1]["passed"]:
            break
    passed = len(results) == args.rounds * 3 and all(r["passed"] for r in results)
    summary = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
               "passed": passed, "requested_rounds": args.rounds, "results": results}
    (args.output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
