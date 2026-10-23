#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p out

fail() {
    echo "FAIL: $1" >&2
    exit 1
}

# TA-01: a single contador VM must print 5, 4, 3, 2, 1 and halt.
./build/minihv -c conf/acceptance-ta01.conf > out/acceptance-ta01-report.txt
actual=$(awk '/contador: [0-9]+$/ { sub(/^.*contador: /, ""); printf "%s ", $0 }' out/acceptance-ta01-report.txt)
[ "$actual" = '5 4 3 2 1 ' ] || fail "TA-01 console output was: $actual"
grep -q 'contador *HALT' out/acceptance-ta01-report.txt || fail 'TA-01 did not HALT'
grep -q ',MEM_FREE,' out/acceptance-ta01.csv || fail 'TA-01 did not release VM memory on exit'

# TA-02: four CPU-bound VMs arrive at ticks 0, 1, 2, and 3.
# The first instruction fetch faults for each VM; SJF's resulting dispatch
# sequence is checked against the hand-calculated deterministic sequence.
./build/minihv -c conf/acceptance-ta02.conf > out/acceptance-ta02-report.txt
actual=$(awk -F, '$5 == "DISPATCH" { printf "%s ", $4 }' out/acceptance-ta02.csv)
expected='cpu0 cpu1 cpu2 cpu0 cpu1 cpu2 cpu3 cpu3 '
[ "$actual" = "$expected" ] || fail "TA-02 dispatch sequence was: $actual"
grep -q 'cpu0 *HALT' out/acceptance-ta02-report.txt || fail 'TA-02 cpu0 did not HALT'
grep -q 'cpu1 *HALT' out/acceptance-ta02-report.txt || fail 'TA-02 cpu1 did not HALT'
grep -q 'cpu2 *HALT' out/acceptance-ta02-report.txt || fail 'TA-02 cpu2 did not HALT'
grep -q 'cpu3 *HALT' out/acceptance-ta02-report.txt || fail 'TA-02 cpu3 did not HALT'

# TA-03: I/O requests, blocking/unblocking, and all application devices.
./build/minihv -c conf/acceptance-ta03.conf > out/acceptance-ta03-report.txt
for device in CONSOLE DISK NET; do
    grep -q ",IO_REQ,device=$device" out/acceptance-ta03.csv || fail "TA-03 missing IO_REQ for $device"
    grep -q ",IO_DONE,device=$device" out/acceptance-ta03.csv || fail "TA-03 missing IO_DONE for $device"
done
grep -q 'from=RUNNING to=BLOCKED' out/acceptance-ta03.csv || fail 'TA-03 no VM blocking transition'
grep -q 'from=BLOCKED to=READY' out/acceptance-ta03.csv || fail 'TA-03 no I/O completion/unblock transition'

# TA-04 and TA-05: the main workload exceeds physical memory, evicts pages,
# and protects the faulting VM without stopping healthy VMs.
./build/minihv -c conf/hv.conf > out/acceptance-main-report.txt
grep -q ',PAGE_FAULT,' out/minihv.csv || fail 'TA-04 missing PAGE_FAULT'
grep -q ',PAGE_EVICT,' out/minihv.csv || fail 'TA-04 missing page replacement'
grep -q 'memory_worker *WATCHDOG' out/acceptance-main-report.txt || fail 'TA-04/08 memory workload did not terminate by WATCHDOG'
grep -q 'guard *PROTECTION_FAULT' out/acceptance-main-report.txt || fail 'TA-05 faulting VM did not receive PROTECTION_FAULT'
grep -q 'web *HALT' out/acceptance-main-report.txt || fail 'TA-05 healthy web VM did not finish'
grep -q 'io_worker *HALT' out/acceptance-main-report.txt || fail 'TA-05 healthy I/O VM did not finish'

# TA-06: an invalid configuration key reports file, line, and key; an invalid
# MV-ISA program is rejected with source file and line instead of crashing.
if ./build/minihv -c conf/invalid-key.conf > out/invalid-config-report.txt 2> out/invalid-config-error.txt; then
    fail 'TA-06 invalid configuration was accepted'
fi
grep -q "conf/invalid-key.conf:3: chave desconhecida 'pcpuz'" out/invalid-config-error.txt || fail 'TA-06 invalid key diagnostic lacks file/line/key'
set +e
./build/minihv -c conf/invalid-program.conf > out/invalid-program-report.txt 2> out/invalid-program-error.txt
invalid_program_status=$?
set -e
[ "$invalid_program_status" -ne 139 ] || fail 'TA-06 invalid program caused SIGSEGV'
[ "$invalid_program_status" -ne 134 ] || fail 'TA-06 invalid program caused abort'
grep -q 'progs/invalid.mv:2: instrucao MV-ISA invalida' out/invalid-program-error.txt || fail 'TA-06 invalid program diagnostic lacks file/line'
grep -q 'INVALID_INSTRUCTION' out/invalid-program-report.txt || fail 'TA-06 invalid program reason not in report'

# C-LOOK order: the pending requests after cylinder 20 are 75, 180, then 10.
./build/minihv -c conf/disk-queue-test.conf > out/disk-queue-report.txt
actual=$(grep ',IO_START,' out/disk-queue-test.csv | grep 'algorithm=C_LOOK' | sed -E 's/.*cylinder=([0-9]+).*/\1/' | tr '\n' ' ')
[ "$actual" = '20 75 180 10 ' ] || fail "C-LOOK order was: $actual"

# TA-08: warning at 5 CPU ticks and termination at 10 for a short test limit.
./build/minihv -c conf/watchdog-test.conf > out/watchdog-report.txt
grep -q ',WATCHDOG_WARN,cpu_ticks_since_io=5 limit=5' out/watchdog-test.csv || fail 'TA-08 WATCHDOG warning threshold incorrect'
grep -q ',WATCHDOG,cpu_ticks_since_io=10 limit=5' out/watchdog-test.csv || fail 'TA-08 WATCHDOG termination threshold incorrect'
grep -q 'infinite *WATCHDOG' out/watchdog-report.txt || fail 'TA-08 infinite VM was not terminated'
grep -q 'counter *HALT' out/watchdog-report.txt || fail 'TA-08 remaining VM did not terminate normally'

# TA-07: the same configuration must produce byte-identical stdout and CSV.
./build/minihv -c conf/hv.conf > out/deterministic-a.txt
cp out/minihv.csv out/deterministic-a.csv
./build/minihv -c conf/hv.conf > out/deterministic-b.txt
cp out/minihv.csv out/deterministic-b.csv
cmp -s out/deterministic-a.txt out/deterministic-b.txt || fail 'TA-07 stdout is not deterministic'
cmp -s out/deterministic-a.csv out/deterministic-b.csv || fail 'TA-07 CSV is not deterministic'

# Interactive CLI smoke test: invalid commands must show help and keep running.
printf 'help\nstep\nps\nmem\nio\nstats\npause contador\nresume contador\nkill contador\nnot_a_command\nquit\n' | \
    ./build/minihv -c conf/acceptance-ta01.conf -i > out/cli-smoke.txt 2>&1
 grep -q 'minihv>' out/cli-smoke.txt || fail 'CLI prompt was not displayed'
 grep -q 'Comando invalido' out/cli-smoke.txt || fail 'CLI invalid command was not diagnosed'
 grep -q 'Comandos MiniHV' out/cli-smoke.txt || fail 'CLI invalid command did not show help'

printf '%s\n' 'TA-01 through TA-08, module unit tests, CLI and C-LOOK regression tests passed.'
printf '%s\n' 'TA-09 must be run with: make valgrind (requires Valgrind installed).'
