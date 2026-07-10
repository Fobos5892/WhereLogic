# WhereLogic - CI / local release build (qmake + mingw32-make + optional tests)
#
# Usage:
#   powershell -NoProfile -File scripts/ci_build.ps1
#   powershell -NoProfile -File scripts/ci_build.ps1 -Configuration release -RunTests
#   powershell -NoProfile -File scripts/ci_build.ps1 -NoOpenCV -RunTests

param(
    [string]$BuildDir = "build-ci",
    [ValidateSet("debug", "release")]
    [string]$Configuration = "release",
    [switch]$NoOpenCV,
    [switch]$RunTests
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$buildPath = Join-Path $repoRoot $BuildDir

function Resolve-Make {
    $make = Get-Command mingw32-make -ErrorAction SilentlyContinue
    if ($make) { return $make.Source }

    $candidates = @(
        "C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe",
        "C:\Qt\Tools\mingw1120_64\bin\mingw32-make.exe"
    )
    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) { return $candidate }
    }
    throw "mingw32-make not found in PATH or C:\Qt\Tools"
}

function Resolve-QMake {
    $qmake = Get-Command qmake -ErrorAction SilentlyContinue
    if ($qmake) { return $qmake.Source }

    $pattern = Join-Path $env:QT_ROOT_DIR "mingw*\bin\qmake.exe"
    $fromEnv = Get-ChildItem -Path $pattern -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($fromEnv) { return $fromEnv.FullName }

    $glob = Get-ChildItem -Path "C:\Qt" -Filter "qmake.exe" -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match "\\mingw.*\\bin\\qmake\.exe$" } |
        Sort-Object FullName -Descending |
        Select-Object -First 1
    if ($glob) { return $glob.FullName }

    throw "qmake not found. Install Qt MinGW or set PATH after install-qt-action."
}

if (Test-Path $buildPath) {
    try {
        Remove-Item -Recurse -Force $buildPath -ErrorAction Stop
    } catch {
        Write-Warning "Could not remove $buildPath - reusing existing build directory."
    }
}
if (-not (Test-Path $buildPath)) {
    New-Item -ItemType Directory -Force -Path $buildPath | Out-Null
}

$qmakeExe = Resolve-QMake
$makeExe = Resolve-Make
$jobs = [int]$env:NUMBER_OF_PROCESSORS
if ($jobs -lt 1) { $jobs = 4 }

$qmakeArgs = @(
    (Join-Path $repoRoot "WhereLogic.pro"),
    "CONFIG+=$Configuration"
)
if ($NoOpenCV) {
    $qmakeArgs += "CONFIG+=no_opencv"
}

Write-Host "Repository: $repoRoot"
Write-Host "Build dir:  $buildPath"
Write-Host "qmake:      $qmakeExe"
Write-Host "make:       $makeExe"
Write-Host "Jobs:       $jobs"
Write-Host "Config:     $Configuration$(if ($NoOpenCV) { ' + no_opencv' })"

Push-Location $buildPath
try {
    & $qmakeExe @qmakeArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    & $makeExe "-j$jobs"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} finally {
    Pop-Location
}

if ($RunTests) {
    $qtBin = Split-Path (Resolve-QMake) -Parent
    $qtRoot = Split-Path $qtBin -Parent
    $env:PATH = "$qtBin;$(Split-Path (Resolve-Make) -Parent);$env:PATH"
    $env:QT_PLUGIN_PATH = Join-Path $qtRoot "plugins"

    $testExes = Get-ChildItem -Path $buildPath -Filter "tst_*.exe" -Recurse -ErrorAction SilentlyContinue |
        Sort-Object FullName
    if (-not $testExes) {
        throw "No test executables found under $buildPath"
    }

    foreach ($testExe in $testExes) {
        Write-Host ""
        Write-Host "Running $($testExe.Name)..."
        $xmlReport = Join-Path $env:TEMP ("wherelogic-" + $testExe.BaseName + ".xml")
        if (Test-Path $xmlReport) { Remove-Item -Force $xmlReport }
        & $testExe.FullName -o "$xmlReport,xml"
        if ($LASTEXITCODE -ne 0) {
            if (Test-Path $xmlReport) {
                Write-Host "--- $($testExe.Name) report ---"
                Get-Content $xmlReport
            }
            throw "Test failed: $($testExe.FullName) (exit $LASTEXITCODE)"
        }
    }
    Write-Host ""
    Write-Host "All tests passed."
}

$gameExe = Get-ChildItem -Path $buildPath -Filter "WhereLogicGame.exe" -Recurse -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
if ($gameExe) {
    Write-Host "Game binary: $($gameExe.FullName)"
} else {
    throw "WhereLogicGame.exe not found under $buildPath"
}
