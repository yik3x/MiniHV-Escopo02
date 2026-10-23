$ErrorActionPreference = 'Stop'
Set-Location (Split-Path -Parent $PSScriptRoot)
if (-not (Test-Path 'build/minihv.exe')) { & '.\scripts\build.ps1' }
& '.\build\minihv.exe' -c 'conf\hv.conf' @args
exit $LASTEXITCODE
