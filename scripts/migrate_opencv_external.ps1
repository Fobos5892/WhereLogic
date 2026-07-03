# Move legacy third_party/opencv/prebuilt → external/WhereLogicOpenCV/prebuilt
#   powershell -ExecutionPolicy Bypass -File scripts/migrate_opencv_external.ps1

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)

$legacy = Join-Path $repoRoot "third_party\opencv\prebuilt"
$target = Join-Path $repoRoot "external\WhereLogicOpenCV\prebuilt"

if (-not (Test-Path $legacy)) {
    Write-Host "Nothing to migrate: $legacy not found"
    exit 0
}

if (Test-Path (Join-Path $target "include\opencv2\opencv.hpp")) {
    Write-Host "Target already has prebuilt: $target"
    exit 0
}

New-Item -ItemType Directory -Force -Path $target | Out-Null

Write-Host "Migrating OpenCV prebuilt..."
Write-Host "  from: $legacy"
Write-Host "  to:   $target"

Get-ChildItem $legacy | Move-Item -Destination $target -Force
Remove-Item $legacy -Recurse -Force -ErrorAction SilentlyContinue

Write-Host "Done. Run qmake and Rebuild All."
