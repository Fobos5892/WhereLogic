param(
    [Parameter(Mandatory = $true)]
    [string]$DestDir,
    [string]$BinDir = ""
)

$ErrorActionPreference = "Stop"

$externalRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)

if (-not $BinDir) {
    $BinDir = Join-Path $externalRoot "prebuilt\x64\mingw\bin"
}

New-Item -ItemType Directory -Force -Path $DestDir | Out-Null

if (-not (Test-Path $BinDir)) {
    Write-Warning "OpenCV bin not found: $BinDir (run scripts/build_qt_mingw.ps1 first)"
    exit 0
}

Get-ChildItem $DestDir -Filter "libopencv_*.dll" -ErrorAction SilentlyContinue | Remove-Item -Force

$modules = @("core", "imgproc", "imgcodecs")
$count = 0
foreach ($mod in $modules) {
    $dll = Get-ChildItem $BinDir -Filter "libopencv_${mod}*.dll" -ErrorAction SilentlyContinue |
        Sort-Object { if ($_.BaseName -match "${mod}\d") { 0 } else { 1 } }, LastWriteTime -Descending |
        Select-Object -First 1
    if ($dll) {
        Copy-Item $dll.FullName $DestDir -Force
        $count++
    }
}

if ($count -eq 0) {
    Write-Warning "No OpenCV DLLs in $BinDir"
    exit 0
}

foreach ($qtRuntime in @("C:\Qt\Tools\mingw1310_64\bin", "C:\Qt\Tools\mingw1120_64\bin")) {
    if (-not (Test-Path $qtRuntime)) {
        continue
    }
    foreach ($name in @("libstdc++-6.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll")) {
        $src = Join-Path $qtRuntime $name
        if (Test-Path $src) {
            Copy-Item $src $DestDir -Force
        }
    }
    break
}

Write-Host "OpenCV: copied $count DLL(s) to $DestDir"
