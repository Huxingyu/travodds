#!/usr/bin/env bash
# Independent, serial TSan build and focused production-path checks.
set -euo pipefail
SOURCE_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
TASK_ROOT=${1:?Usage: tsan.sh /absolute/path/to/output-root}
mkdir -p "$TASK_ROOT"
TASK_ROOT=$(cd "$TASK_ROOT" && pwd)
mkdir -p "$TASK_ROOT/logs" "$TASK_ROOT/results"
export TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1
FLAGS='-O1 -g1 -fsanitize=thread -fno-omit-frame-pointer'
RUN=()
# Per-process workaround for the diagnosed host address-space conflict. No
# global kernel setting is changed. Disable on hosts that do not need it.
if [[ ${TSAN_NO_ASLR:-1} == 1 ]]; then RUN=(setarch "$(uname -m)" -R); fi
run_logged() {
    local name=$1
    shift
    local status=0
    "$@" > "$TASK_ROOT/logs/$name.log" 2>&1 || status=$?
    echo "$status" > "$TASK_ROOT/results/$name.exit"
    if [[ $status != 0 ]]; then
        tail -70 "$TASK_ROOT/logs/$name.log"
        return "$status"
    fi
}
run_logged configure cmake -S "$SOURCE_ROOT" -B "$TASK_ROOT/build" \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
    -DCMAKE_CXX_FLAGS_DEBUG="$FLAGS" -DCMAKE_SHARED_LINKER_FLAGS=-fsanitize=thread \
    -DSECURITY=OFF -DCMAKE_INSTALL_PREFIX="$TASK_ROOT/install"
run_logged build "${RUN[@]}" cmake --build "$TASK_ROOT/build" --parallel 1
run_logged install cmake --install "$TASK_ROOT/build"
run_logged configure-tests cmake -S "$SOURCE_ROOT/test/unit_payloadpool" -B "$TASK_ROOT/build-tests" \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
    -DCMAKE_CXX_FLAGS_DEBUG="$FLAGS -fno-pie" -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=thread -no-pie' \
    -DCMAKE_PREFIX_PATH="$TASK_ROOT/install"
run_logged build-tests "${RUN[@]}" cmake --build "$TASK_ROOT/build-tests" --parallel 1
run_logged test-timer timeout 180s "${RUN[@]}" "$TASK_ROOT/build-tests/payloadpool_test" \
    --gtest_filter=TimerSynchronization.*:LoggerSynchronization.* --gtest_repeat=30 \
    "--gtest_output=xml:$TASK_ROOT/results/timer.xml"
run_logged test-qos timeout 180s "${RUN[@]}" "$TASK_ROOT/build-tests/payloadpool_test" \
    '--gtest_filter=LoanIntegration.ConcurrentWriterQos*:LoanIntegration.ConcurrentReaderQos*:LoanIntegration.*DeadlineListenerCanQuery*:LoanIntegration.TimeBasedFilterListener*:LoanIntegration.KeyedAutopurgeStateRecheck*:NativeReaderIntegration.KeyedFinitePurge*' \
    --gtest_repeat=10 "--gtest_output=xml:$TASK_ROOT/results/qos.xml"
printf 'Focused TSan checks passed: %s\n' "$TASK_ROOT"
