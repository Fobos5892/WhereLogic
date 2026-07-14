# WhereLogic - portable package for another PC (no Qt/OpenCV/MSYS2 install required)
#
# Usage (from repo root, after Release build):
#   powershell -ExecutionPolicy Bypass -File scripts/package_portable.ps1
#   powershell -ExecutionPolicy Bypass -File scripts/package_portable.ps1 -ExePath "build\...\release\WhereLogicGame.exe"
#
# Output: dist/WhereLogicGame-portable/ - copy this entire folder to another PC.

param(
    [string]$ExePath = "",
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)

function Find-GameExe {
    if ($ExePath -and (Test-Path $ExePath)) { return (Resolve-Path $ExePath).Path }
    $searchRoots = @(
        (Join-Path $Root "build"),
        (Join-Path $Root "build-ci")
    )
    foreach ($searchRoot in $searchRoots) {
        if (-not (Test-Path $searchRoot)) { continue }
        $candidates = Get-ChildItem -Path $searchRoot -Filter "WhereLogicGame.exe" -Recurse -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match "\\release\\" -or $_.FullName -match "\\Release\\" } |
            Sort-Object LastWriteTime -Descending
        if ($candidates) { return $candidates[0].FullName }
    }
    foreach ($searchRoot in $searchRoots) {
        if (-not (Test-Path $searchRoot)) { continue }
        $candidates = Get-ChildItem -Path $searchRoot -Filter "WhereLogicGame.exe" -Recurse -ErrorAction SilentlyContinue |
            Sort-Object LastWriteTime -Descending
        if ($candidates) { return $candidates[0].FullName }
    }
    return $null
}

function Find-QtBinDir {
    $qmake = Get-Command qmake -ErrorAction SilentlyContinue
    if ($qmake) { return (Split-Path $qmake.Source) }

    if ($env:QT_ROOT_DIR) {
        $bin = Join-Path $env:QT_ROOT_DIR "bin"
        if (Test-Path (Join-Path $bin "windeployqt.exe")) { return $bin }
    }

    $roots = @()
    if ($env:IQTA_TOOLS) {
        $roots += (Split-Path $env:IQTA_TOOLS -Parent)
    }
    $roots += @("C:\Qt", "D:\Qt")

    foreach ($root in $roots) {
        if (-not (Test-Path $root)) { continue }
        $hit = Get-ChildItem -Path $root -Filter "windeployqt.exe" -Recurse -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match "\\6\.[0-9].*\\mingw[^\\]*\\bin\\windeployqt\.exe$" } |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        if ($hit) { return $hit.DirectoryName }
    }
    return $null
}

function Find-WinDeployQt {
    $qtBin = Find-QtBinDir
    if (-not $qtBin) { return $null }
    $candidate = Join-Path $qtBin "windeployqt.exe"
    if (Test-Path $candidate) { return $candidate }
    return $null
}

function Find-MingwRuntimeDir {
    $candidates = @()
    if ($env:IQTA_TOOLS) {
        $candidates += (Join-Path $env:IQTA_TOOLS "mingw1310_64\bin")
        $candidates += (Join-Path $env:IQTA_TOOLS "mingw1120_64\bin")
    }
    $qtBin = Find-QtBinDir
    if ($qtBin) {
        # Qt\6.11.1\mingw_64\bin -> Qt\Tools\mingw1310_64\bin
        $qtRoot = Split-Path (Split-Path (Split-Path $qtBin -Parent) -Parent) -Parent
        $candidates += (Join-Path $qtRoot "Tools\mingw1310_64\bin")
        $candidates += (Join-Path $qtRoot "Tools\mingw1120_64\bin")
        $candidates += (Resolve-Path (Join-Path $qtBin "..\..\..\Tools\mingw*\bin") -ErrorAction SilentlyContinue |
            Select-Object -ExpandProperty Path)
    }
    $candidates += @(
        "C:\Qt\Tools\mingw1310_64\bin",
        "C:\Qt\Tools\mingw1120_64\bin"
    )
    foreach ($dir in ($candidates | Where-Object { $_ } | Select-Object -Unique)) {
        if (Test-Path (Join-Path $dir "libstdc++-6.dll")) { return $dir }
    }
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
if (-not $windeploy) {
    throw @"
windeployqt.exe not found.
Add Qt MinGW bin to PATH (so qmake/windeployqt work), or set QT_ROOT_DIR.
Example:
  `$env:Path = 'C:\Qt\6.11.1\mingw_64\bin;' + `$env:Path
  powershell -ExecutionPolicy Bypass -File scripts\package_portable.ps1
"@
}

$qmlDir = Join-Path $Root "game\qml"
Write-Host "windeployqt: $windeploy"
$deployArgs = @(
    "--release",
    "--compiler-runtime",
    "--qmldir", $qmlDir,
    (Join-Path $OutDir "WhereLogicGame.exe")
)
& $windeploy @deployArgs
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt failed with exit code $LASTEXITCODE"
}

$requiredQtDll = Join-Path $OutDir "Qt6Core.dll"
if (-not (Test-Path $requiredQtDll)) {
    throw "Qt6Core.dll missing in $OutDir after windeployqt - portable package is incomplete."
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
$mingwDir = Find-MingwRuntimeDir
foreach ($name in $mingwDlls) {
    $dest = Join-Path $OutDir $name
    if (Test-Path $dest) { continue }

    $fromExe = Join-Path $exeDir $name
    if (Test-Path $fromExe) {
        Copy-Item $fromExe $OutDir -Force
        continue
    }
    if ($mingwDir) {
        $fromTool = Join-Path $mingwDir $name
        if (Test-Path $fromTool) {
            Copy-Item $fromTool $OutDir -Force
        }
    }
}

$missingMingw = @($mingwDlls | Where-Object { -not (Test-Path (Join-Path $OutDir $_)) })
if ($missingMingw.Count -gt 0) {
    Write-Warning "Missing MinGW runtime DLL(s): $($missingMingw -join ', '). Game may fail on PCs without MinGW."
}

# Data / config next to exe (optional)
$configSrc = Join-Path $Root "config"
if (Test-Path $configSrc) {
    Copy-Item $configSrc (Join-Path $OutDir "config") -Recurse -Force
}

$dllCount = @(Get-ChildItem $OutDir -Filter "*.dll" -File -ErrorAction SilentlyContinue).Count
Write-Host ""
Write-Host "Done. Portable folder ($dllCount DLLs):"
Write-Host "  $OutDir"
Write-Host ""
Write-Host "Copy the ENTIRE folder to another PC and run WhereLogicGame.exe."
Write-Host "No MSYS2, OpenCV, or Qt install required on the target machine."
Write-Host ""
Write-Host "Without OpenCV at build time (CONFIG+=no_opencv): same script, smaller folder, masks disabled."
