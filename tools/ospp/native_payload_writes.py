#!/usr/bin/env python3
"""Audit completed native-payload page-protection evidence from a real DDS run.

The artifact proves a software memory-access invariant for the exercised native
operations. It does not measure hardware DRAM writes or promise that a hostile
process cannot bypass a test guard by arbitrary backing-file I/O.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import signal
import sys
import xml.etree.ElementTree as ET


PREFIX = "DS_PAYLOAD_WRITE_PROOF "
SUITE = "NativePayloadWrites"
CASES = {
    "UnkeyedFrameworkCannotAccessProtectedNativePayload": "unkeyed",
    "KeyedFrameworkReadsOnlyTheIndependentKeyPage": "keyed",
    "GuardCatchesAnInjectedConstructorPayloadStore": "constructor_store_probe",
    "GuardCatchesAnInjectedReaderPayloadStore": "reader_store_probe",
}
PROCESS_FIELDS = {
    "pid", "device_major", "device_minor", "inode", "file_offset", "bytes",
    "virtual_address", "aliases", "guarded_checks", "app_write_windows",
    "app_read_windows", "constructions", "serialize_calls", "deserialize_calls",
    "copy_calls", "create_calls", "seccomp_tsync", "blocked_mprotect",
    "blocked_file_mmap", "blocked_mremap", "read_guard_faults", "initial_payload_read_only", "loan_pages",
    "loan_discard_reloan", "published", "read_returned", "taken",
    "writer_deleted_protected", "reader_deleted_protected", "loan_released_protected",
}
RECORD_FIELDS = {
    "schema", "case", "scenario", "page_bytes", "sample_bytes", "key_page_bytes",
    "framework_body_access", "writer", "reader", "fault_signal",
    "exact_orphan_removed", "hardware_dram_write_count_measured",
}
IDENTITY_FIELDS = ("device_major", "device_minor", "inode", "file_offset", "bytes")
FALLBACK_FIELDS = ("serialize_calls", "deserialize_calls", "copy_calls", "create_calls")
PAGE_FIELDS = {
    "virtual_address", "vma_start", "vma_end", "vma_file_offset",
    "device_major", "device_minor", "inode", "permissions",
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def exact_keys(value: object, expected: set[str], label: str) -> dict:
    require(type(value) is dict, f"{label}: expected an object")
    actual = set(value)
    require(actual == expected,
            f"{label}: unexpected fields {sorted(actual - expected)}, "
            f"missing fields {sorted(expected - actual)}")
    return value


def exact(value: object, expected: object, label: str) -> None:
    require(type(value) is type(expected) and value == expected,
            f"{label}: expected {expected!r}, got {value!r}")


def unique_json_fields(pairs: list[tuple[str, object]]) -> dict:
    result = {}
    for key, value in pairs:
        require(key not in result, f"proof JSON contains duplicate field {key!r}")
        result[key] = value
    return result


def xml_cases(path: Path) -> dict[str, dict]:
    root = ET.parse(path).getroot()
    suites = [suite for suite in root.iter("testsuite") if suite.get("name") == SUITE]
    require(len(suites) == 1, f"XML must contain exactly one {SUITE} suite")
    suite = suites[0]
    for field in ("failures", "errors", "disabled", "skipped"):
        require(suite.get(field, "0") == "0", f"XML {SUITE} has {field}")
    tests = suite.findall("testcase")
    require(len(tests) == len(CASES), f"XML must contain exactly {len(CASES)} {SUITE} cases")
    require(suite.get("tests") == str(len(CASES)), "XML suite test count is inconsistent")
    result = {}
    for test in tests:
        name = test.get("name")
        require(name in CASES and name not in result, f"unexpected or duplicate XML case {name!r}")
        require(test.get("classname") == SUITE, f"XML {name}: wrong classname")
        require(test.get("status") == "run" and test.get("result") == "completed",
                f"XML {name}: case did not run to completion")
        require(not any(child.tag in {"failure", "error", "skipped"} for child in test),
                f"XML {name}: failed, errored, or skipped case")
        result[name] = {"status": "completed", "seconds": test.get("time")}
    require(set(result) == set(CASES), "XML is missing required payload-protection cases")
    return result


def log_records(path: Path) -> dict[str, dict]:
    records = {}
    for line_number, line in enumerate(path.read_text(encoding="utf-8", errors="strict").splitlines(), 1):
        if PREFIX not in line:
            continue
        require(line.startswith(PREFIX), f"log line {line_number}: proof prefix must start the line")
        record = json.loads(line[len(PREFIX):], object_pairs_hook=unique_json_fields)
        exact_keys(record, RECORD_FIELDS, f"log line {line_number}")
        full_name = record["case"]
        require(type(full_name) is str and full_name.startswith(SUITE + "."),
                f"log line {line_number}: wrong proof suite")
        name = full_name[len(SUITE) + 1:]
        require(name in CASES and name not in records,
                f"log line {line_number}: unknown or duplicate proof case {full_name!r}")
        records[name] = record
    require(set(records) == set(CASES),
            f"log missing required proof cases {sorted(set(CASES) - set(records))}")
    return records


def process_shape(proof: object, label: str) -> dict:
    proof = exact_keys(proof, PROCESS_FIELDS, label)
    for field, value in proof.items():
        if field == "loan_pages":
            continue
        require(type(value) is int and value >= 0, f"{label}.{field}: expected a nonnegative integer")
    require(type(proof["loan_pages"]) is list and len(proof["loan_pages"]) == 3,
            f"{label}.loan_pages: expected exactly three actual page VMA witnesses")
    for index, page in enumerate(proof["loan_pages"]):
        exact_keys(page, PAGE_FIELDS, f"{label}.loan_pages[{index}]")
        for field, value in page.items():
            if field == "permissions":
                require(type(value) is str, f"{label}.loan_pages[{index}].permissions: expected a string")
            else:
                require(type(value) is int and value >= 0,
                        f"{label}.loan_pages[{index}].{field}: expected a nonnegative integer")
    return proof


def process_guard(proof: dict, reader: bool, label: str) -> None:
    for field in ("pid", "inode", "file_offset", "virtual_address", "aliases", "guarded_checks"):
        require(proof[field] > 0, f"{label}.{field}: missing actual guarded mapping evidence")
    require(proof["file_offset"] % 4096 == 0 and proof["virtual_address"] % 4096 == 0,
            f"{label}: actual Loan and file offset must be page aligned")
    exact(proof["bytes"], 12288, f"{label}.bytes")
    for field in ("seccomp_tsync", "blocked_mprotect", "blocked_file_mmap", "blocked_mremap", "read_guard_faults"):
        exact(proof[field], 1, f"{label}.{field}")
    exact(proof["initial_payload_read_only"], int(reader), f"{label}.initial_payload_read_only")
    for field in FALLBACK_FIELDS:
        exact(proof[field], 0, f"{label}.{field}")


def actual_page_witnesses(proof: dict, keyed: bool, label: str) -> None:
    for index, page in enumerate(proof["loan_pages"]):
        address = proof["virtual_address"] + index * 4096
        exact(page["virtual_address"], address, f"{label}.loan_pages[{index}].virtual_address")
        require(page["vma_start"] <= address and address + 4096 <= page["vma_end"],
                f"{label}.loan_pages[{index}]: actual Loan page is not covered by its recorded VMA")
        for field in ("vma_start", "vma_end", "vma_file_offset"):
            require(page[field] % 4096 == 0, f"{label}.loan_pages[{index}].{field}: not page aligned")
        require(page["vma_file_offset"] + address - page["vma_start"] == proof["file_offset"] + index * 4096,
                f"{label}.loan_pages[{index}]: actual VMA does not recompute to the expected backing-file offset")
        for field in ("device_major", "device_minor", "inode"):
            exact(page[field], proof[field], f"{label}.loan_pages[{index}].{field}")
        exact(page["permissions"], "r--s" if keyed and index == 0 else "---s",
              f"{label}.loan_pages[{index}].permissions")


def require_empty_process(proof: dict, label: str) -> None:
    require(all(value == 0 for field, value in proof.items() if field != "loan_pages"),
            f"{label}: unexpectedly populated process proof")
    for page in proof["loan_pages"]:
        require(all(value == ("" if field == "permissions" else 0) for field, value in page.items()),
                f"{label}: unexpectedly populated page witness")


def phases(proof: dict, expected: dict[str, int], label: str) -> None:
    for field in (
        "app_write_windows", "app_read_windows", "constructions", "loan_discard_reloan",
        "published", "read_returned", "taken", "writer_deleted_protected",
        "reader_deleted_protected", "loan_released_protected",
    ):
        exact(proof[field], expected.get(field, 0), f"{label}.{field}")


def validate_record(name: str, record: dict) -> dict:
    label = f"{SUITE}.{name}"
    scenario = CASES[name]
    for field, expected in {
        "schema": 1, "case": label, "scenario": scenario, "page_bytes": 4096,
        "sample_bytes": 12288, "key_page_bytes": 4096 if scenario == "keyed" else 0,
        "framework_body_access": "PROT_NONE", "hardware_dram_write_count_measured": False,
        "exact_orphan_removed": scenario == "constructor_store_probe",
        "fault_signal": int(signal.SIGSEGV) if scenario.endswith("_probe") else 0,
    }.items():
        exact(record[field], expected, f"{label}.{field}")
    writer = process_shape(record["writer"], label + ".writer")
    reader = process_shape(record["reader"], label + ".reader")
    process_guard(writer, False, label + ".writer")
    actual_page_witnesses(writer, scenario == "keyed", label + ".writer")
    if scenario == "constructor_store_probe":
        phases(writer, {"constructions": 1}, label + ".writer")
        require_empty_process(reader, label + ".reader")
    else:
        phases(writer, {
            "app_write_windows": 1, "constructions": 2, "loan_discard_reloan": 1,
            "published": 1, "writer_deleted_protected": 1,
        }, label + ".writer")
        process_guard(reader, True, label + ".reader")
        actual_page_witnesses(reader, scenario == "keyed", label + ".reader")
        phases(reader, {
            "app_read_windows": 1 if scenario == "reader_store_probe" else 3,
            "read_returned": 1, "taken": 1,
            "reader_deleted_protected": int(scenario != "reader_store_probe"),
            "loan_released_protected": int(scenario != "reader_store_probe"),
        }, label + ".reader")
        require(writer["pid"] != reader["pid"], f"{label}: endpoints were not independent processes")
        require(all(writer[field] == reader[field] for field in IDENTITY_FIELDS),
                f"{label}: actual Writer and Reader Loans do not share the same backing bytes")
    return {
        "case": label, "scenario": scenario, "passed": True,
        "writer": writer, "reader": reader,
        "shared_backing_witness": {field: writer[field] for field in IDENTITY_FIELDS},
        "fault_signal": record["fault_signal"],
        "exact_orphan_removed": record["exact_orphan_removed"],
    }


def file_evidence(path: Path) -> dict:
    return {"path": str(path.resolve()), "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--xml", type=Path, required=True, help="completed gtest XML (other suites permitted)")
    parser.add_argument("--log", type=Path, required=True, help="raw test stdout/stderr containing proof records")
    parser.add_argument("--output", type=Path, required=True, help="new JSON artifact path; never overwritten")
    args = parser.parse_args()
    try:
        require(not args.output.exists(), f"output already exists: {args.output}")
        completed = xml_cases(args.xml)
        records = log_records(args.log)
        cases = [validate_record(name, records[name]) for name in CASES]
        for case in cases:
            case["test_result"] = completed[case["case"].split(".", 1)[1]]
        source = Path(__file__).resolve().parents[2] / "test/unit_payloadpool/test_native_payload_writes.cpp"
        summary = {
            "schema": 1,
            "passed": True,
            "status": "passed",
            "suite": SUITE,
            "tests": len(cases),
            "software_payload_access_invariant": "passed_for_exercised_guarded_native_operations",
            "same_device_inode_file_offsets": "passed_for_actual_cross_process_Loans",
            "initial_reader_payload_mapping": "PROT_READ_before_test_guard_installation",
            "hardware_dram_write_count_measured": False,
            "scope": [
                "Actual independent-process DDS native Loan/read/take path; complete body pages are PROT_NONE outside application windows.",
                "Writer initializes the complete sample in one application RW window; Reader application validation uses RO windows.",
                "Keyed framework access is limited to a separate RO key page; all body pages remain inaccessible.",
                "Protected phases include construction, discard/reloan, publish, read/return_loan, take, endpoint deletion and final pin release.",
                "All existing payload aliases are guarded; process-wide TSYNC seccomp rejects ordinary permission changes and new file-backed aliases.",
                "Raw per-page VMA witnesses independently recompute the actual Loan addresses to the same device, inode and file offsets.",
                "Injected body loads fault in every participating process; independent injected constructor and Reader stores also fault.",
            ],
            "limitations": [
                "This is a software access and shared-backing witness, not a hardware DRAM-write counter or physical-page-number measurement.",
                "Mappings exist before the test closes access; segment creation before guard installation needs the separate production-source review.",
                "The guard is not a hostile-code security boundary and does not deny every possible backing-file read/write mechanism; production payload-file I/O needs separate source review.",
                "Evidence is for Linux x86_64 with 4096-byte pages and these exercised operations, type layouts, and process lifecycles.",
            ],
            "cases": cases,
            "inputs": {
                "xml": file_evidence(args.xml), "log": file_evidence(args.log),
                "auditor": file_evidence(Path(__file__)), "test_source": file_evidence(source),
            },
        }
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with args.output.open("x", encoding="utf-8") as output:
            json.dump(summary, output, indent=2, ensure_ascii=False)
            output.write("\n")
        print(f"native payload write/access proof: {len(cases)} cases passed; {args.output}")
        return 0
    except (OSError, ValueError, TypeError, ET.ParseError, KeyError) as error:
        print(f"native payload write/access proof rejected: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
