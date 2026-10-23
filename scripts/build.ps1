$ErrorActionPreference = 'Stop'
Set-Location (Split-Path -Parent $PSScriptRoot)
New-Item -ItemType Directory -Force -Path 'build', 'out' | Out-Null
$sources = Get-ChildItem -Path 'src' -Filter '*.c' -Recurse | ForEach-Object { $_.FullName }
if (-not (Get-Command gcc -ErrorAction SilentlyContinue)) {
    throw 'gcc nao foi encontrado. Instale GCC/MinGW-w64 ou use CMake com um compilador C11.'
}
& gcc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Isrc $sources -o 'build/minihv.exe'
if ($LASTEXITCODE -ne 0) { throw "Compilacao falhou (codigo $LASTEXITCODE)." }
Write-Host 'Build concluido: build/minihv.exe'
Write-Host 'Execute: .\build\minihv.exe -c conf\hv.conf'
