# WhereLogic — portable package for another PC (no Qt/OpenCV/MSYS2 install required)
#
# Usage (from repo root, after Release build):
#   powershell -ExecutionPolicy Bypass -File scripts/package_portable.ps1
#   powershell -ExecutionPolicy Bypass -File scripts/package_portable.ps1 -ExePath "build\...\release\WhereLogicGame.exe"
#
# Output: dist/WhereLogicGame-portable/  — copy this entire folder to another PC.

param(
    [string]$ExePath = "",
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)

function Find-GameExe {
    if ($ExePath -and (Test-Path $ExePath)) { return (Resolve-Path $ExePath).Path }
    $candidates = Get-ChildItem -Path (Join-Path $Root "build") -Filter "WhereLogicGame.exe" -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match "\\release\\" -or $_.FullName -match "\\Release\\" } |
        Sort-Object LastWriteTime -Descending
    if ($candidates) { return $candidates[0].FullName }
    $candidates = Get-ChildItem -Path (Join-Path $Root "build") -Filter "WhereLogicGame.exe" -Recurse -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending
    if ($candidates) { return $candidates[0].FullName }
    return $null
}

function Find-WinDeployQt {
    $qmake = Get-Command qmake -ErrorAction SilentlyContinue
    if (-not $qmake) { return $null }
    $qtBin = Split-Path $qmake.Source
    $candidate = Join-Path $qtBin "windeployqt.exe"
    if (Test-Path $candidate) { return $candidate }
    return $null
}

$exe = Find-GameExe
if (-not $exe) {
    Write-Error "WhereLogicGame.exe not found. Build Release first or pass -ExePath."
}

$exeDir = Split-Path $exe
if (-not $OutDir) {
    $OutDir = Join-Path $Root "dist\WhereLogicGame-portable"
}
if (Test-Path $OutDir) { Remove-Item -Recurse -Force $OutDir }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

Write-Host "Source exe: $exe"
Write-Host "Output:     $OutDir"

Copy-Item $exe $OutDir

# Qt + QML plugins
$windeploy = Find-WinDeployQt
if ($windeploy) {
    $qmlDir = Join-Path $Root "game\qml"
    Write-Host "Running windeployqt..."
    & $windeploy --release --qmldir $qmlDir (Join-Path $OutDir "WhereLogicGame.exe")
} else {
    Write-Warning "windeployqt not in PATH — copy Qt DLLs manually or add Qt bin to PATH."
}

# OpenCV runtime (if built with HAS_OPENCV)
$opencvBin = Join-Path $Root "external\WhereLogicOpenCV\prebuilt\x64\mingw\bin"
if (-not (Test-Path $opencvBin)) {
    $opencvBin = Join-Path $Root "external\WhereLogicOpenCV\prebuilt\x64\msvc\bin"
}
if (Test-Path $opencvBin) {
    Write-Host "Copying OpenCV DLLs from $opencvBin"
    Copy-Item (Join-Path $opencvBin "*.dll") $OutDir -Force
}

# MinGW runtime (needed for MinGW-built exe on PC without compiler)
$mingwDlls = @("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll")
foreach ($name in $mingwDlls) {
    $fromExe = Join-Path $exeDir $name
    if (Test-Path $fromExe) {
        Copy-Item $fromExe $OutDir -Force
        continue
    }
    $qmake = Get-Command qmake -ErrorAction SilentlyContinue
    if ($qmake) {
        $qtBin = Split-Path $qmake.Source
        $toolchain = Resolve-Path (Join-Path $qtBin "..\..\Tools\mingw*\bin") -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($toolchain) {
            $fromTool = Join-Path $toolchain $name
            if (Test-Path $fromTool) { Copy-Item $fromTool $OutDir -Force }
        }
    }
}

# Optional MSYS2 OpenCV dependencies — not needed for Qt-built WhereLogicOpenCV
$msysBin = "C:\msys64\mingw64\bin"
if (Test-Path $msysBin) {
    $extra = @("libjpeg-*.dll", "libpng*.dll", "libtiff-*.dll", "libwebp-*.dll", "zlib1.dll", "libopenjp2-*.dll")
    foreach ($pat in $extra) {
        Get-ChildItem (Join-Path $msysBin $pat) -ErrorAction SilentlyContinue | ForEach-Object {
            Copy-Item $_.FullName $OutDir -Force
        }
    }
}

# Data / config next to exe (optional)
$configSrc = Join-Path $Root "config"
if (Test-Path $configSrc) {
    Copy-Item $configSrc (Join-Path $OutDir "config") -Recurse -Force
}

Write-Host ""
Write-Host "Done. Portable folder:"
Write-Host "  $OutDir"
Write-Host ""
Write-Host "Copy the ENTIRE folder to another PC and run WhereLogicGame.exe."
Write-Host "No MSYS2, OpenCV, or Qt install required on the target machine."
Write-Host ""
Write-Host "Without OpenCV at build time (CONFIG+=no_opencv): same script, smaller folder, masks disabled."
