$ErrorActionPreference = 'Stop'
Set-Location (Split-Path -Parent $PSScriptRoot)
if (-not (Test-Path 'build/minihv.exe')) { & '.\scripts\build.ps1' }
New-Item -ItemType Directory -Force -Path 'build', 'out' | Out-Null

# Build and execute one assert-based test executable for each module.
$projectSources = @(Get-ChildItem 'src' -Recurse -Filter '*.c' |
    Where-Object { $_.Name -ne 'main.c' } |
    ForEach-Object { $_.FullName })
$unitTests = @(Get-ChildItem 'tests' -Filter 'test_*.c' | Sort-Object Name)
foreach ($testSource in $unitTests) {
    $testName = [System.IO.Path]::GetFileNameWithoutExtension($testSource.Name)
    $testExe = Join-Path (Get-Location) (Join-Path 'build' ($testName + '.exe'))
    $gccArgs = @('-std=c11', '-Wall', '-Wextra', '-Werror', '-pedantic', '-O2', '-Isrc', $testSource.FullName) + $projectSources + @('-o', $testExe)
    & gcc @gccArgs
    if ($LASTEXITCODE -ne 0) { throw "Compiling $testName failed (code $LASTEXITCODE)." }
    if ($testName -eq 'test_config') {
    # This unit intentionally triggers an invalid-key diagnostic on stderr.
    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $testExe 2> 'out\test-config-diagnostic.txt'
        $testStatus = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
} else {
    & $testExe
    $testStatus = $LASTEXITCODE
}
if ($testStatus -ne 0) { throw "$testName failed (code $testStatus)." }

}
$exe = '.\build\minihv.exe'

function Assert-True([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw "FAIL: $Message" }
}
function Run-Case([string]$Config, [string]$OutputPath) {
    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $exe -c $Config *> $OutputPath
        $runStatus = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
    Assert-True ($runStatus -eq 0) "execution failed for $Config (code $runStatus)"
}
function Read-Events([string]$Path) {
    return @(Import-Csv -Path $Path)
}

# TA-01: single contador VM prints 5, 4, 3, 2, 1 and halts.
Run-Case 'conf\acceptance-ta01.conf' 'out\acceptance-ta01-report.txt'
$ta01 = Get-Content 'out\acceptance-ta01-report.txt' -Raw
$numbers = @([regex]::Matches($ta01, '(?m)^\[t=[0-9]+\] contador: ([0-9]+)\r?$') | ForEach-Object { $_.Groups[1].Value })
Assert-True (($numbers -join ',') -eq '5,4,3,2,1') "TA-01 console values were: $($numbers -join ',')"
Assert-True ($ta01 -match 'contador\s+HALT') 'TA-01 did not HALT'
$memFreeEvents = @(Read-Events 'out\acceptance-ta01.csv' | Where-Object { $_.event -eq 'MEM_FREE' })
Assert-True ($memFreeEvents.Count -gt 0) 'TA-01 did not release VM memory'

# TA-02: four CPU-bound VMs with different arrival ticks.
Run-Case 'conf\acceptance-ta02.conf' 'out\acceptance-ta02-report.txt'
$ta02events = Read-Events 'out\acceptance-ta02.csv'
$dispatch = @($ta02events | Where-Object { $_.event -eq 'DISPATCH' } | ForEach-Object { $_.vm })
Assert-True (($dispatch -join ',') -eq 'cpu0,cpu1,cpu2,cpu0,cpu1,cpu2,cpu3,cpu3') "TA-02 dispatch sequence was: $($dispatch -join ',')"
$ta02 = Get-Content 'out\acceptance-ta02-report.txt' -Raw
foreach ($vm in @('cpu0','cpu1','cpu2','cpu3')) { Assert-True ($ta02 -match "$vm\s+HALT") "TA-02 $vm did not HALT" }

# TA-03: requests and completions for all application I/O devices.
Run-Case 'conf\acceptance-ta03.conf' 'out\acceptance-ta03-report.txt'
$ta03events = Read-Events 'out\acceptance-ta03.csv'
foreach ($device in @('CONSOLE','DISK','NET')) {
    Assert-True (@($ta03events | Where-Object { $_.event -eq 'IO_REQ' -and $_.detail -match "device=$device" }).Count -gt 0) "TA-03 missing IO_REQ for $device"
    Assert-True (@($ta03events | Where-Object { $_.event -eq 'IO_DONE' -and $_.detail -match "device=$device" }).Count -gt 0) "TA-03 missing IO_DONE for $device"
}
Assert-True (@($ta03events | Where-Object { $_.event -eq 'STATE_CHANGE' -and $_.detail -match 'from=RUNNING to=BLOCKED' }).Count -gt 0) 'TA-03 missing BLOCKED transition'
Assert-True (@($ta03events | Where-Object { $_.event -eq 'STATE_CHANGE' -and $_.detail -match 'from=BLOCKED to=READY' }).Count -gt 0) 'TA-03 missing READY interrupt transition'

# TA-04 and TA-05: paging/replacement, isolated protection fault, healthy VMs finish.
Run-Case 'conf\hv.conf' 'out\acceptance-main-report.txt'
$main = Get-Content 'out\acceptance-main-report.txt' -Raw
$mainEvents = Read-Events 'out\minihv.csv'
Assert-True (@($mainEvents | Where-Object { $_.event -eq 'PAGE_FAULT' }).Count -gt 0) 'TA-04 missing PAGE_FAULT'
Assert-True (@($mainEvents | Where-Object { $_.event -eq 'PAGE_EVICT' }).Count -gt 0) 'TA-04 missing page replacement'
Assert-True ($main -match 'guard\s+PROTECTION_FAULT') 'TA-05 protection fault missing'
Assert-True ($main -match 'web\s+HALT') 'TA-05 healthy web VM did not finish'
Assert-True ($main -match 'io_worker\s+HALT') 'TA-05 healthy I/O VM did not finish'


# TA-06: invalid key and invalid program syntax must be reported with line numbers.
$previousErrorActionPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    & $exe -c 'conf\invalid-key.conf' *> 'out\invalid-config-error.txt'
    $invalidConfigStatus = $LASTEXITCODE
}
finally {
    $ErrorActionPreference = $previousErrorActionPreference
}
Assert-True ($invalidConfigStatus -ne 0) 'TA-06 invalid configuration was accepted'
$invalidConfigError = Get-Content 'out\invalid-config-error.txt' -Raw
Assert-True ($invalidConfigError -match "invalid-key\.conf:3: chave desconhecida 'pcpuz'") 'TA-06 invalid-key diagnostic lacks file/line/key'
Run-Case 'conf\invalid-program.conf' 'out\invalid-program-report.txt'
$invalidProgramError = Get-Content 'out\invalid-program-report.txt' -Raw
Assert-True ($invalidProgramError -match 'progs/invalid\.mv:2: instrucao MV-ISA invalida') 'TA-06 invalid-program source diagnostic missing'
Assert-True ($invalidProgramError -match 'INVALID_INSTRUCTION') 'TA-06 invalid-program exit reason missing'

# C-LOOK regression: pending cylinders must be served in 20, 75, 180, 10 order.
Run-Case 'conf\disk-queue-test.conf' 'out\disk-queue-report.txt'
$diskEvents = Read-Events 'out\disk-queue-test.csv'
$cylinders = @($diskEvents | Where-Object { $_.event -eq 'IO_START' -and $_.detail -match 'algorithm=C_LOOK' } | ForEach-Object { if ($_.detail -match 'cylinder=(\d+)') { $Matches[1] } })
Assert-True (($cylinders -join ',') -eq '20,75,180,10') "C-LOOK order was: $($cylinders -join ',')"

# TA-08: warning at 5 CPU ticks, termination at 10, another VM finishes normally.
Run-Case 'conf\watchdog-test.conf' 'out\watchdog-report.txt'
$watchdogEvents = Read-Events 'out\watchdog-test.csv'
Assert-True (@($watchdogEvents | Where-Object { $_.event -eq 'WATCHDOG_WARN' -and $_.detail -match 'cpu_ticks_since_io=5 limit=5' }).Count -gt 0) 'TA-08 WATCHDOG warning threshold incorrect'
Assert-True (@($watchdogEvents | Where-Object { $_.event -eq 'WATCHDOG' -and $_.detail -match 'cpu_ticks_since_io=10 limit=5' }).Count -gt 0) 'TA-08 WATCHDOG exit threshold incorrect'
$watchdog = Get-Content 'out\watchdog-report.txt' -Raw
Assert-True ($watchdog -match 'infinite\s+WATCHDOG') 'TA-08 infinite VM was not terminated'
Assert-True ($watchdog -match 'counter\s+HALT') 'TA-08 other VM did not finish'

# TA-07: same config must yield byte-identical stdout and CSV.
Run-Case 'conf\hv.conf' 'out\deterministic-a.txt'
Copy-Item 'out\minihv.csv' 'out\deterministic-a.csv' -Force
Run-Case 'conf\hv.conf' 'out\deterministic-b.txt'
Copy-Item 'out\minihv.csv' 'out\deterministic-b.csv' -Force
Assert-True ((Get-FileHash 'out\deterministic-a.txt').Hash -eq (Get-FileHash 'out\deterministic-b.txt').Hash) 'TA-07 stdout is not deterministic'
Assert-True ((Get-FileHash 'out\deterministic-a.csv').Hash -eq (Get-FileHash 'out\deterministic-b.csv').Hash) 'TA-07 CSV is not deterministic'

# Interactive CLI smoke test: invalid commands must show help and keep running.
$interactiveCommands = @('help', 'step', 'ps', 'mem', 'io', 'stats', 'pause contador', 'resume contador', 'kill contador', 'not_a_command', 'quit')
$previousErrorActionPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    $interactiveCommands | & $exe -c 'conf\acceptance-ta01.conf' -i *> 'out\cli-smoke.txt'
    $interactiveStatus = $LASTEXITCODE
}
finally {
    $ErrorActionPreference = $previousErrorActionPreference
}
Assert-True ($interactiveStatus -eq 0) 'interactive CLI smoke test failed'
$cliOutput = Get-Content 'out\cli-smoke.txt' -Raw
Assert-True ($cliOutput -match 'minihv>') 'interactive prompt missing'
Assert-True ($cliOutput -match 'Comando invalido') 'invalid command did not print a diagnostic'
Assert-True ($cliOutput -match 'Comandos MiniHV') 'invalid command did not show help'

Write-Host 'Module tests and TA-01 through TA-08 / C-LOOK regression tests passed.'
Write-Host 'TA-09 requires Valgrind on a supported Unix-like environment; this PowerShell script does not claim Valgrind validation.'
