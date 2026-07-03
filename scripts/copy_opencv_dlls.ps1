# Deprecated — use external/WhereLogicOpenCV/scripts/copy_dlls.ps1
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$script = Join-Path $root "external\WhereLogicOpenCV\scripts\copy_dlls.ps1"
if (-not (Test-Path $script)) {
    Write-Error "WhereLogicOpenCV not found. Clone into external/WhereLogicOpenCV first."
}
& $script @args
