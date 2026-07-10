# WhereLogic - pack project + portable game + AppData into one ZIP for a flash drive.
#
# Usage (from repo root, after Release build for portable/):
#   powershell -ExecutionPolicy Bypass -File scripts/pack_bundle.ps1
#   powershell -ExecutionPolicy Bypass -File scripts/pack_bundle.ps1 -Output "E:\WhereLogic-backup.zip"
#   powershell -ExecutionPolicy Bypass -File scripts/pack_bundle.ps1 -SkipPortable -SkipUserData

param(
    [string]$Output = "",
    [switch]$SkipPortable,
    [switch]$SkipUserData
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Stamp = Get-Date -Format "yyyy-MM-dd"
$BundleName = "WhereLogic-bundle-$Stamp"
$Stage = Join-Path $env:TEMP $BundleName

if (Test-Path $Stage) { Remove-Item -Recurse -Force $Stage }
New-Item -ItemType Directory -Force -Path $Stage | Out-Null

function Invoke-RobocopyProject {
    param([string]$Src, [string]$Dst)
    $excludeDirs = @(
        "build",
        "dist",
        ".qtcreator",
        ".build"
    )
    $excludeFiles = @("*.pro.user*", "*.autosave", "Thumbs.db", "desktop.ini")
    & robocopy $Src $Dst /E `
        /XD $excludeDirs `
        /XF $excludeFiles `
        /NFL /NDL /NJH /NJS /nc /ns /np | Out-Null
    if ($LASTEXITCODE -ge 8) {
        throw "robocopy failed with exit code $LASTEXITCODE"
    }
}

Write-Host "Staging bundle in: $Stage"

# --- 1. Project sources (no build artifacts) ---
$ProjectDst = Join-Path $Stage "project"
New-Item -ItemType Directory -Force -Path $ProjectDst | Out-Null
Write-Host "Copying project..."
Invoke-RobocopyProject -Src $Root -Dst $ProjectDst

# --- 2. Portable runnable game (no Qt install on target PC) ---
$hasPortable = $false
if (-not $SkipPortable) {
    $PortableScript = Join-Path $Root "scripts\package_portable.ps1"
    $PortableDst = Join-Path $Stage "portable"
    if (Test-Path $PortableScript) {
        Write-Host "Building portable folder..."
        try {
            & $PortableScript -OutDir $PortableDst
            if (Test-Path (Join-Path $PortableDst "WhereLogicGame.exe")) {
                $hasPortable = $true
            }
        } catch {
            Write-Warning "Portable build skipped (build Release first or pass -SkipPortable): $_"
            if (Test-Path $PortableDst) { Remove-Item -Recurse -Force $PortableDst }
        }
    } else {
        Write-Warning "package_portable.ps1 not found - portable/ omitted."
    }
} else {
    Write-Host "Skipping portable/ -SkipPortable."
}

# --- 3. User data (puzzles, presets) from AppData ---
$hasUserData = $false
if (-not $SkipUserData) {
    $AppDataSrc = Join-Path $env:APPDATA "WhereLogic"
    if (Test-Path $AppDataSrc) {
        Write-Host "Copying user data from: $AppDataSrc"
        Copy-Item $AppDataSrc (Join-Path $Stage "userdata") -Recurse -Force
        $hasUserData = $true
    } else {
        Write-Warning "AppData not found: $AppDataSrc"
    }
} else {
    Write-Host "Skipping userdata/ -SkipUserData."
}

# --- README inside archive ---
$readme = @'
WhereLogic bundle
=================

Contents:
  project/   - source tree (open in Qt Creator on a PC with Qt)
  portable/  - runnable game: WhereLogicGame.exe, no Qt needed
  userdata/  - puzzles and settings from %AppData%\WhereLogic

Restore on another PC:
  powershell -ExecutionPolicy Bypass -File project\scripts\unpack_bundle.ps1 -Archive <this.zip> -Target D:\WhereLogic

Or unpack manually:
  - Run portable\WhereLogicGame.exe for play-only
  - Copy userdata\ to %AppData%\WhereLogic to restore your puzzles
'@ -replace 'WhereLogic bundle', "WhereLogic bundle ($Stamp)"
Set-Content -Path (Join-Path $Stage "README-BUNDLE.txt") -Value $readme -Encoding UTF8

# --- Manifest ---
$manifest = [ordered]@{
    formatVersion = 1
    created       = (Get-Date).ToString("o")
    machine       = $env:COMPUTERNAME
    hasPortable   = $hasPortable
    hasUserData   = $hasUserData
    hasProject    = $true
}
$manifest | ConvertTo-Json | Set-Content (Join-Path $Stage "manifest.json") -Encoding UTF8

# --- ZIP ---
if (-not $Output) {
    $Output = Join-Path $Root "dist\$BundleName.zip"
}
$OutParent = Split-Path $Output -Parent
if (-not (Test-Path $OutParent)) { New-Item -ItemType Directory -Force -Path $OutParent | Out-Null }
if (Test-Path $Output) { Remove-Item -Force $Output }

Write-Host "Creating archive: $Output"
Compress-Archive -Path (Join-Path $Stage "*") -DestinationPath $Output -CompressionLevel Optimal
Remove-Item -Recurse -Force $Stage

Write-Host ""
Write-Host "Done."
Write-Host "  Archive: $Output"
Write-Host "  Portable: $(if ($hasPortable) { 'yes' } else { 'no - build Release and re-run' })"
Write-Host "  User data: $(if ($hasUserData) { 'yes' } else { 'no' })"
Write-Host ""
Write-Host "Copy the ZIP to a flash drive. Unpack with scripts\unpack_bundle.ps1"
