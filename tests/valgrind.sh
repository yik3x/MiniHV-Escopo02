#!/bin/sh
set -u
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p out

if ! command -v valgrind >/dev/null 2>&1; then
    echo 'Valgrind is not installed; TA-09 cannot be verified.' >&2
    exit 2
fi

run_case() {
    name=$1
    expected=$2
    shift 2
    set +e
    valgrind --leak-check=full --show-leak-kinds=definite \
        --errors-for-leak-kinds=definite --error-exitcode=99 \
        --log-file="out/valgrind-$name.log" "$@" >/dev/null 2>/dev/null
    status=$?
    set -e
    [ "$status" -eq "$expected" ] || { echo "FAIL: Valgrind/runtime status for $name was $status (expected $expected)" >&2; cat "out/valgrind-$name.log" >&2; exit 1; }
    if grep -q 'All heap blocks were freed -- no leaks are possible' "out/valgrind-$name.log"; then
    :
elif grep -q 'definitely lost: 0 bytes in 0 blocks' "out/valgrind-$name.log"; then
    :
else
    echo "FAIL: unable to confirm absence of definite leaks in $name" >&2
    cat "out/valgrind-$name.log" >&2
    exit 1
fi
}

set -e
run_case ta01 0 ./build/minihv -c conf/acceptance-ta01.conf
run_case ta02 0 ./build/minihv -c conf/acceptance-ta02.conf
run_case ta03 0 ./build/minihv -c conf/acceptance-ta03.conf
run_case ta04_ta05 0 ./build/minihv -c conf/hv.conf
run_case ta07_a 0 ./build/minihv -c conf/hv.conf
run_case ta07_b 0 ./build/minihv -c conf/hv.conf
run_case ta06_config 1 ./build/minihv -c conf/invalid-key.conf
run_case ta06_program 0 ./build/minihv -c conf/invalid-program.conf
run_case ta08_watchdog 0 ./build/minihv -c conf/watchdog-test.conf
run_case disk_clook 0 ./build/minihv -c conf/disk-queue-test.conf
printf '%s\n' 'TA-09 passed: Valgrind found no definitely-lost memory in all acceptance scenarios.'
