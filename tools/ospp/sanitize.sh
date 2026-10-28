#!/usr/bin/env bash
# Independently instrument the library and pool tests; keep every build serial.
set -euo pipefail
SOURCE_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
TASK_ROOT=${1:?Usage: sanitize.sh /absolute/path/to/output-root}
mkdir -p "$TASK_ROOT"
TASK_ROOT=$(cd "$TASK_ROOT" && pwd)
CMAKE_BIN=${CMAKE_BIN:-cmake}
mkdir -p "$TASK_ROOT/logs" "$TASK_ROOT/results"
# Test discovery also executes instrumented binaries. A host with restricted
# ptrace may require ASAN_DETECT_LEAKS=0; this does not disable ASan/UBSan.
export ASAN_OPTIONS="detect_leaks=${ASAN_DETECT_LEAKS:-1}:halt_on_error=1"
export UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1

run_logged() {
    local name=$1
    shift
    if ! "$@" > "$TASK_ROOT/logs/$name.log" 2>&1; then
        tail -60 "$TASK_ROOT/logs/$name.log"
        return 1
    fi
}

FLAGS='-O1 -g1 -fsanitize=address,undefined -fno-omit-frame-pointer'
if [[ ${COVERAGE:-0} == 1 ]]; then FLAGS="$FLAGS --coverage"; fi
clear_coverage() {
    python3 - "$TASK_ROOT/build" "$TASK_ROOT/build-tests" "$TASK_ROOT/build-core" "$TASK_ROOT/build-data_sharing" <<'PY'
from pathlib import Path
import sys
for root in sys.argv[1:]:
    for counters in Path(root).rglob('*.gcda'):
        counters.unlink()
PY
}
# Discovery loads the rebuilt instrumented library too. Remove previous
# source checksums before it executes, then discard discovery-only counts.
if [[ ${COVERAGE:-0} == 1 ]]; then clear_coverage; fi
run_logged configure "$CMAKE_BIN" -S "$SOURCE_ROOT" -B "$TASK_ROOT/build" \
    -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS_DEBUG="$FLAGS" \
    -DCMAKE_SHARED_LINKER_FLAGS=-fsanitize=address,undefined \
    -DSECURITY=OFF -DCMAKE_INSTALL_PREFIX="$TASK_ROOT/install"
run_logged build python3 "$SOURCE_ROOT/tools/ospp/measure.py" \
    "$TASK_ROOT/results/build-resources.json" \
    "$CMAKE_BIN" --build "$TASK_ROOT/build" --parallel 1
run_logged install "$CMAKE_BIN" --install "$TASK_ROOT/build"
run_logged configure-tests "$CMAKE_BIN" -S "$SOURCE_ROOT/test/unit_payloadpool" \
    -B "$TASK_ROOT/build-tests" -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS_DEBUG="$FLAGS -fno-pie" \
    -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie' \
    -DCMAKE_PREFIX_PATH="$TASK_ROOT/install"
run_logged build-tests "$CMAKE_BIN" --build "$TASK_ROOT/build-tests" --parallel 1
run_logged configure-data_sharing "$CMAKE_BIN" -S "$SOURCE_ROOT/example/cpp/data_sharing" \
    -B "$TASK_ROOT/build-data_sharing" -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS_DEBUG="$FLAGS -fno-pie" \
    -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie' \
    -DCMAKE_PREFIX_PATH="$TASK_ROOT/install"
run_logged build-data_sharing "$CMAKE_BIN" --build "$TASK_ROOT/build-data_sharing" --parallel 1
if [[ ${CORE_TESTS:-0} == 1 ]]; then
    run_logged configure-core "$CMAKE_BIN" -S "$SOURCE_ROOT/test/unit_inside" \
        -B "$TASK_ROOT/build-core" -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
        -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS_DEBUG="$FLAGS -fno-pie" \
        -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie' \
        -DCMAKE_PREFIX_PATH="$TASK_ROOT/install"
    run_logged build-core "$CMAKE_BIN" --build "$TASK_ROOT/build-core" --parallel 1
fi
if [[ ${COVERAGE:-0} == 1 ]]; then
    clear_coverage
fi
cd "$TASK_ROOT/results"
if [[ ${CORE_TESTS:-0} == 1 ]]; then
    run_logged test-core timeout 650s "$TASK_ROOT/build-core/unit_test" \
        "--gtest_output=xml:$TASK_ROOT/results/core.xml"
fi
for name in payloadpool sharedpayloadpool; do
    run_logged "test-$name" timeout 120s "$TASK_ROOT/build-tests/${name}_test" \
        "--gtest_output=xml:$TASK_ROOT/results/$name.xml"
done
printf 'Sanitizer build and selected tests passed: %s\n' "$TASK_ROOT"
