#!/usr/bin/env python3
"""Merge GCC JSON line counts and measure added production lines.

The denominator is added executable lines in src/ and include/, as identified
by GCC. It excludes unchanged lines, tests, dependencies and documentation.
"""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import subprocess


def added_lines(source, baseline):
    result = {}
    diff = subprocess.check_output(
        ["git", "diff", baseline, "--unified=0", "--no-ext-diff"], cwd=source,
    ).decode("utf-8", errors="replace")
    name, line = None, None
    for text in diff.splitlines():
        if text.startswith("+++ b/"):
            name, line = text[6:], None
        elif text.startswith("@@"):
            match = re.match(r"@@ -\d+(?:,\d+)? \+(\d+)(?:,\d+)? @@", text)
            line = int(match.group(1)) if match else None
        elif name is not None and line is not None:
            if text.startswith("+"):
                result.setdefault(name, set()).add(line)
                line += 1
            elif text.startswith(" "):
                line += 1
    untracked = subprocess.check_output(
        ["git", "ls-files", "--others", "--exclude-standard", "-z"], cwd=source,
    ).decode().split("\0")
    for name in filter(None, untracked):
        path = source / name
        if path.suffix in (".cpp", ".h") and name.startswith(("src/", "include/")):
            result[name] = set(range(1, len(path.read_bytes().splitlines()) + 1))
    return {name: lines for name, lines in result.items()
            if name.startswith(("src/", "include/")) and Path(name).suffix in (".cpp", ".h")}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--gcov", default="gcov")
    parser.add_argument("--minimum", type=float, default=70)
    parser.add_argument("--baseline", default="HEAD", help="Commit used to identify added production lines")
    args = parser.parse_args()
    source, build = args.source.resolve(), args.build_root.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    baseline = subprocess.check_output(
        ["git", "rev-parse", "--verify", "--end-of-options", args.baseline + "^{commit}"], cwd=source,
    ).decode().strip()
    changed = added_lines(source, baseline)
    counts = {name: {} for name in changed}
    processed = 0
    logs = []
    # Read gcno too: objects not exercised by the tests still contribute zero
    # counts. Translation units merge by source line, including templates.
    for notes in sorted(build.rglob("*.gcno")):
        if "_deps" in notes.parts:
            continue
        unit = output / hashlib.sha256(str(notes).encode()).hexdigest()[:16]
        unit.mkdir(exist_ok=True)
        run = subprocess.run([args.gcov, "--json-format", str(notes)], cwd=unit,
                             capture_output=True, text=True)
        logs.append(f"{notes}\n{run.stdout}{run.stderr}")
        if run.returncode:
            raise RuntimeError(logs[-1])
        processed += 1
        for report in unit.glob("*.gcov.json.gz"):
            with gzip.open(report, "rt") as handle:
                data = json.load(handle)
            cwd = Path(data.get("current_working_directory", source))
            for file in data["files"]:
                path = Path(file["file"])
                if not path.is_absolute():
                    path = cwd / path
                try:
                    name = str(path.resolve().relative_to(source))
                except ValueError:
                    continue
                if name not in counts:
                    continue
                for entry in file["lines"]:
                    line = entry["line_number"]
                    counts[name][line] = counts[name].get(line, 0) + entry["count"]
    details = {}
    # Every changed .cpp must have been compiled and collected. Otherwise GCC's
    # executable-line intersection could silently remove an entire module.
    missing = [name for name in changed if name.endswith('.cpp') and not counts[name]]
    if missing:
        raise RuntimeError('Changed production translation units missing from coverage: ' + repr(missing))
    for name, lines in sorted(changed.items()):
        executable = sorted(lines.intersection(counts[name]))
        # GCC may infer a negative line count from inconsistent profile arcs.
        # Such a value is not evidence that the line executed.
        uncovered = [line for line in executable if counts[name][line] <= 0]
        details[name] = {
            "added_lines": len(lines), "executable_added_lines": len(executable),
            "covered_added_lines": len(executable) - len(uncovered),
            "uncovered_added_lines": uncovered,
            "source_sha256": hashlib.sha256((source / name).read_bytes()).hexdigest(),
        }
    total = sum(file["executable_added_lines"] for file in details.values())
    covered = sum(file["covered_added_lines"] for file in details.values())
    if not processed or not total:
        raise RuntimeError("No instrumented production changes found")
    ratio = 100 * covered / total
    summary = {
        "baseline": baseline,
        "metric": "GCC executable production lines added since baseline, merged across translation units",
        "translation_units": processed, "covered_added_lines": covered,
        "executable_added_lines": total, "percent": round(ratio, 3),
        "required_percent": args.minimum, "passed": ratio >= args.minimum,
        "missing_changed_translation_units": missing,
        "files": details,
    }
    (output / "gcov.log").write_text("\n".join(logs))
    (output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps({key: value for key, value in summary.items() if key != "files"}, indent=2))
    return 0 if summary["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
