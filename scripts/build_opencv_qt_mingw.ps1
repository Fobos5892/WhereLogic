# Deprecated — use external/WhereLogicOpenCV/scripts/build_qt_mingw.ps1
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$script = Join-Path $root "external\WhereLogicOpenCV\scripts\build_qt_mingw.ps1"
if (-not (Test-Path $script)) {
    Write-Error "WhereLogicOpenCV not found. Clone into external/WhereLogicOpenCV first."
}
Write-Host "Redirecting to external/WhereLogicOpenCV/scripts/build_qt_mingw.ps1"
& $script @args
