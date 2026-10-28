#!/usr/bin/env python3
"""Run a command and record Linux child resource usage without GNU time."""
import datetime
import json
from pathlib import Path
import resource
import subprocess
import sys
import time

output = Path(sys.argv[1])
command = sys.argv[2:]
started = time.monotonic()
result = subprocess.run(command, check=False)
usage = resource.getrusage(resource.RUSAGE_CHILDREN)
output.write_text(json.dumps({
    "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
    "command": command,
    "exit_code": result.returncode,
    "wall_seconds": round(time.monotonic() - started, 3),
    "user_seconds": usage.ru_utime,
    "system_seconds": usage.ru_stime,
    "max_child_rss_kib": usage.ru_maxrss,
    "rss_note": "Linux maximum individual child RSS; not the aggregate parallel build memory",
}, indent=2) + "\n")
raise SystemExit(result.returncode)
