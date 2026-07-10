# WhereLogic - unpack a bundle ZIP created by pack_bundle.ps1
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File scripts/unpack_bundle.ps1 -Archive "E:\WhereLogic-backup.zip"
#   powershell -ExecutionPolicy Bypass -File scripts/unpack_bundle.ps1 -Archive "...\zip" -Target "D:\WhereLogic"
#   powershell -ExecutionPolicy Bypass -File scripts/unpack_bundle.ps1 -Archive "...\zip" -NoRestoreUserData

param(
    [Parameter(Mandatory = $true)]
    [string]$Archive,
    [string]$Target = "",
    [switch]$RestoreUserData,
    [switch]$NoRestoreUserData
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $Archive)) {
    throw "Archive not found: $Archive"
}

if (-not $Target) {
    $Target = Join-Path $env:USERPROFILE "WhereLogic-restore"
}

$ExtractRoot = Join-Path $env:TEMP ("WhereLogic-unpack-" + [guid]::NewGuid().ToString("n"))
New-Item -ItemType Directory -Force -Path $ExtractRoot | Out-Null

try {
    Write-Host "Extracting: $Archive"
    Expand-Archive -Path $Archive -DestinationPath $ExtractRoot -Force

    $BundleRoot = $ExtractRoot
    if (-not (Test-Path (Join-Path $BundleRoot "manifest.json"))) {
        $inner = Get-ChildItem $ExtractRoot -Directory | Select-Object -First 1
        if ($inner -and (Test-Path (Join-Path $inner.FullName "manifest.json"))) {
            $BundleRoot = $inner.FullName
        }
    }

    $manifestPath = Join-Path $BundleRoot "manifest.json"
    if (Test-Path $manifestPath) {
        $manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
        Write-Host "Bundle from $($manifest.created) on $($manifest.machine)"
    }

    $ProjectSrc = $null
    foreach ($name in @("WhereLogic", "project", "repo")) {
        $candidate = Join-Path $BundleRoot $name
        if (Test-Path (Join-Path $candidate "WhereLogic.pro")) {
            $ProjectSrc = $candidate
            break
        }
    }
    if (-not $ProjectSrc) {
        throw "No source tree in archive (expected WhereLogic/ with WhereLogic.pro)"
    }
    $PortableSrc = Join-Path $BundleRoot "portable"
    $UserSrc = Join-Path $BundleRoot "userdata"

    if (Test-Path $ProjectSrc) {
        if (Test-Path $Target) {
            Write-Warning "Target exists, replacing: $Target"
            Remove-Item -Recurse -Force $Target
        }
        Copy-Item $ProjectSrc $Target -Recurse -Force
        Write-Host "Project restored: $Target"
    } else {
        Write-Warning "No WhereLogic/ source folder in archive - only portable/userdata if present."
    }

    if (Test-Path (Join-Path $PortableSrc "WhereLogicGame.exe")) {
        $PortableDst = Join-Path $Target "run-portable"
        if (Test-Path $PortableDst) { Remove-Item -Recurse -Force $PortableDst }
        Copy-Item $PortableSrc $PortableDst -Recurse -Force
        Write-Host ("Portable game: " + (Join-Path $PortableDst "WhereLogicGame.exe"))
    }

    $shouldRestoreUserData = $RestoreUserData
    if (-not $NoRestoreUserData -and -not $shouldRestoreUserData -and (Test-Path $UserSrc)) {
        $answer = Read-Host "Restore puzzles/settings to AppData? [Y/n]"
        $shouldRestoreUserData = ($answer -eq "" -or $answer -match "^[Yy]")
    }

    if ($shouldRestoreUserData -and (Test-Path $UserSrc)) {
        $AppDataDst = Join-Path $env:APPDATA "WhereLogic"
        if (Test-Path $AppDataDst) {
            $Backup = "$AppDataDst.backup-" + (Get-Date -Format "yyyyMMdd-HHmmss")
            Copy-Item $AppDataDst $Backup -Recurse -Force
            Write-Host "Previous AppData backed up: $Backup"
        }
        New-Item -ItemType Directory -Force -Path (Split-Path $AppDataDst) | Out-Null
        Copy-Item $UserSrc $AppDataDst -Recurse -Force
        Write-Host "User data restored: $AppDataDst"
    } elseif ($NoRestoreUserData) {
        Write-Host "User data not restored -NoRestoreUserData."
    }

    Write-Host ""
    Write-Host "Done."
    $playExe = Join-Path $Target "run-portable\WhereLogicGame.exe"
    if (Test-Path $playExe) {
        Write-Host ("  Play:   " + $playExe)
    }
    if (Test-Path $Target) {
        Write-Host ('  Develop: open ' + $Target + ' in Qt Creator - Qt required')
    }
} finally {
    if (Test-Path $ExtractRoot) {
        Remove-Item -Recurse -Force $ExtractRoot
    }
}
