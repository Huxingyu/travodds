#!/usr/bin/env bash
# Build into a task-private prefix, without sudo or global library installation.
set -euo pipefail
SOURCE_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
TASK_ROOT=${1:?Usage: bootstrap.sh /absolute/path/to/output-root}
mkdir -p "$TASK_ROOT"
TASK_ROOT=$(cd "$TASK_ROOT" && pwd)
CMAKE_BIN=${CMAKE_BIN:-cmake}
BUILD_JOBS=${BUILD_JOBS:-1}
CC_BIN=${CC_BIN:-/usr/bin/gcc}
CXX_BIN=${CXX_BIN:-/usr/bin/g++}
mkdir -p "$TASK_ROOT/logs" "$TASK_ROOT/results"

run_logged() {
    local name=$1
    shift
    if ! "$@" > "$TASK_ROOT/logs/$name.log" 2>&1; then
        tail -60 "$TASK_ROOT/logs/$name.log"
        return 1
    fi
}

run_logged configure "$CMAKE_BIN" -S "$SOURCE_ROOT" -B "$TASK_ROOT/build" \
    -DCMAKE_C_COMPILER="$CC_BIN" -DCMAKE_CXX_COMPILER="$CXX_BIN" \
    -DCMAKE_BUILD_TYPE=Release -DSECURITY=OFF \
    -DCMAKE_INSTALL_PREFIX="$TASK_ROOT/install"
run_logged build python3 "$SOURCE_ROOT/tools/ospp/measure.py" \
    "$TASK_ROOT/results/build-resources.json" \
    "$CMAKE_BIN" --build "$TASK_ROOT/build" --parallel "$BUILD_JOBS"
run_logged install "$CMAKE_BIN" --install "$TASK_ROOT/build"

for name in hello_world transmit_shm data_sharing; do
    run_logged "configure-$name" "$CMAKE_BIN" \
        -S "$SOURCE_ROOT/example/cpp/$name" -B "$TASK_ROOT/build-$name" \
        -DCMAKE_CXX_COMPILER="$CXX_BIN" -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$TASK_ROOT/install"
    run_logged "build-$name" "$CMAKE_BIN" --build "$TASK_ROOT/build-$name" \
        --parallel "$BUILD_JOBS"
done

run_logged build-udp_only python3 "$SOURCE_ROOT/tools/ospp/udp_example.py" \
    --source "$SOURCE_ROOT" --prefix "$TASK_ROOT/install" \
    --output "$TASK_ROOT/build-udp_only" --cxx "$CXX_BIN"

for name in unit_inside unit_payloadpool; do
    if [[ -d "$SOURCE_ROOT/test/$name" ]]; then
        run_logged "configure-$name" "$CMAKE_BIN" \
            -S "$SOURCE_ROOT/test/$name" -B "$TASK_ROOT/build-$name" \
            -DCMAKE_CXX_COMPILER="$CXX_BIN" -DCMAKE_PREFIX_PATH="$TASK_ROOT/install"
        run_logged "build-$name" "$CMAKE_BIN" --build "$TASK_ROOT/build-$name" \
            --parallel "$BUILD_JOBS"
    fi
done
printf 'Build complete. Logs: %s/logs; tests must be run separately.\n' "$TASK_ROOT"
