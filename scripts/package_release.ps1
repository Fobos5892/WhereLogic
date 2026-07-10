# WhereLogic - portable release folder + ZIP for CI / GitHub Releases
#
# Usage (after Release build):
#   powershell -NoProfile -File scripts/package_release.ps1
#   powershell -NoProfile -File scripts/package_release.ps1 -Version "1.2.0" -BuildDir build-ci

param(
    [string]$Version = "",
    [string]$BuildDir = "build-ci",
    [string]$ExePath = "",
    [string]$OutZip = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$portableScript = Join-Path $repoRoot "scripts\package_portable.ps1"

if (-not (Test-Path $portableScript)) {
    throw "Missing $portableScript"
}

if ([string]::IsNullOrWhiteSpace($Version)) {
    if ($env:GITHUB_REF_NAME) {
        $Version = $env:GITHUB_REF_NAME
    } elseif ($env:GITHUB_SHA) {
        $Version = $env:GITHUB_SHA.Substring(0, 7)
    } else {
        $Version = Get-Date -Format "yyyy-MM-dd"
    }
}
$Version = ($Version -replace '^refs/tags/', '' -replace '^v', '').Trim()
if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = Get-Date -Format "yyyy-MM-dd"
}

if ([string]::IsNullOrWhiteSpace($ExePath)) {
    $buildPath = Join-Path $repoRoot $BuildDir
    $gameCandidates = Get-ChildItem -Path $buildPath -Filter "WhereLogicGame.exe" -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match "\\release\\" -or $_.FullName -match "\\Release\\" } |
        Sort-Object LastWriteTime -Descending
    if (-not $gameCandidates) {
        $gameCandidates = Get-ChildItem -Path $buildPath -Filter "WhereLogicGame.exe" -Recurse -ErrorAction SilentlyContinue |
            Sort-Object LastWriteTime -Descending
    }
    if (-not $gameCandidates) {
        throw "WhereLogicGame.exe not found under $buildPath. Build Release first."
    }
    $ExePath = $gameCandidates[0].FullName
}

$portableDir = Join-Path $repoRoot "dist\WhereLogicGame-portable"
& $portableScript -ExePath $ExePath -OutDir $portableDir

$zipBase = "WhereLogicGame-$Version-win64-portable"
if ([string]::IsNullOrWhiteSpace($OutZip)) {
    $OutZip = Join-Path $repoRoot "dist\$zipBase.zip"
}
$zipParent = Split-Path $OutZip -Parent
if (-not (Test-Path $zipParent)) {
    New-Item -ItemType Directory -Force -Path $zipParent | Out-Null
}
if (Test-Path $OutZip) {
    Remove-Item -Force $OutZip
}

$readme = @"
WhereLogic $Version - Windows portable (MinGW 64-bit)
====================================================

Run WhereLogicGame.exe from this folder.
Copy the entire folder to another PC - Qt and OpenCV are included.

User data (puzzles, settings) is stored separately in:
  %AppData%\WhereLogic\
"@
Set-Content -Path (Join-Path $portableDir "README-PORTABLE.txt") -Value $readme -Encoding UTF8

Write-Host "Creating ZIP: $OutZip"
Compress-Archive -Path (Join-Path $portableDir "*") -DestinationPath $OutZip -CompressionLevel Optimal

$shaPath = "$OutZip.sha256"
$hash = Get-FileHash -Path $OutZip -Algorithm SHA256
Set-Content -Path $shaPath -Value ("{0}  {1}" -f $hash.Hash, (Split-Path $OutZip -Leaf)) -Encoding ASCII

Write-Host ""
Write-Host "Portable folder: $portableDir"
Write-Host "Release archive: $OutZip"
Write-Host "Checksum:        $shaPath"

if ($env:GITHUB_OUTPUT) {
    Add-Content -Path $env:GITHUB_OUTPUT -Value "zip_path=$OutZip"
    Add-Content -Path $env:GITHUB_OUTPUT -Value "portable_dir=$portableDir"
}
