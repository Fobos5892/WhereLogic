# WhereLogic - local backup: full source code + OpenCV prebuilt + game + AppData.
#
# Usage (from repo root):
#   powershell -ExecutionPolicy Bypass -File scripts/pack_bundle.ps1
#
# Output in dist/:
#   WhereLogic-kit/WhereLogic/        - FULL source tree (game/, scripts/, .pro, OpenCV prebuilt)
#   WhereLogic-kit/portable/          - runnable game
#   WhereLogic-kit/userdata/          - puzzles from AppData
#   WhereLogic-bundle-YYYY-MM-DD.zip  - archive of the kit
#
# Optional: -SkipPortable -SkipUserData -SkipOpenCV -SkipZip -SkipGit

param(
    [string]$Output = "",
    [switch]$SkipPortable,
    [switch]$SkipUserData,
    [switch]$SkipOpenCV,
    [switch]$SkipZip,
    [switch]$SkipGit
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$DistDir = Join-Path $Root "dist"
$KitDir = Join-Path $DistDir "WhereLogic-kit"
$PortableDir = Join-Path $DistDir "WhereLogicGame-portable"
$Stamp = Get-Date -Format "yyyy-MM-dd"
$BundleName = "WhereLogic-bundle-$Stamp"

function Test-OpenCvPrebuilt {
    param([string]$BaseRoot)
    $prebuilt = Join-Path $BaseRoot "external\WhereLogicOpenCV\prebuilt"
    $stamp = Join-Path $prebuilt ".opencv_built"
    $header = Join-Path $prebuilt "include\opencv2\opencv.hpp"
    $libDir = Join-Path $prebuilt "x64\mingw\lib"
    $coreLib = Get-ChildItem -Path $libDir -Filter "libopencv_core*.dll.a" -ErrorAction SilentlyContinue |
        Select-Object -First 1
    return (Test-Path $stamp) -and (Test-Path $header) -and $null -ne $coreLib
}

function Ensure-OpenCvPrebuilt {
    if (Test-OpenCvPrebuilt -BaseRoot $Root) {
        Write-Host "OpenCV prebuilt: OK"
        return
    }
    $buildScript = Join-Path $Root "external\WhereLogicOpenCV\scripts\build_qt_mingw.ps1"
    if (-not (Test-Path $buildScript)) {
        throw "OpenCV prebuilt is missing and build script was not found: $buildScript"
    }
    Write-Host "OpenCV prebuilt missing - building (may take ~15 min)..."
    & $buildScript
    if (-not (Test-OpenCvPrebuilt -BaseRoot $Root)) {
        throw "OpenCV prebuilt build finished but required files are still missing."
    }
    Write-Host "OpenCV prebuilt: built"
}

function Invoke-RobocopyProject {
    param([string]$Src, [string]$Dst)
    $excludeDirs = @(
        "build",
        "build-ci",
        "dist",
        ".qtcreator",
        ".build"
    )
    if ($SkipGit) {
        $excludeDirs += ".git"
    }
    $excludeFiles = @("*.pro.user*", "*.autosave", "Thumbs.db", "desktop.ini")
    & robocopy $Src $Dst /E `
        /XD $excludeDirs `
        /XF $excludeFiles `
        /NFL /NDL /NJH /NJS /nc /ns /np | Out-Null
    if ($LASTEXITCODE -ge 8) {
        throw "robocopy failed with exit code $LASTEXITCODE"
    }
}

function Assert-SourceTree {
    param([string]$SourceRoot)
    $required = @(
        "WhereLogic.pro",
        "game\WhereLogicGame.pro",
        "game\src",
        "game\qml",
        "scripts\pack_bundle.ps1",
        "config\ui_defaults_ru.json",
        "external\WhereLogicOpenCV"
    )
    foreach ($rel in $required) {
        $path = Join-Path $SourceRoot $rel
        if (-not (Test-Path $path)) {
            throw "Source copy incomplete - missing: $rel"
        }
    }
    Write-Host "Source tree verified: $SourceRoot"
}

function Sync-OpenCvPrebuiltIntoProject {
    param([string]$ProjectRoot)
    $src = Join-Path $Root "external\WhereLogicOpenCV\prebuilt"
    $dst = Join-Path $ProjectRoot "external\WhereLogicOpenCV\prebuilt"
    if (-not (Test-Path $src)) {
        throw "OpenCV prebuilt source folder not found: $src"
    }
    New-Item -ItemType Directory -Force -Path (Split-Path $dst -Parent) | Out-Null
    if (Test-Path $dst) {
        Remove-Item -Recurse -Force $dst
    }
    Write-Host "Copying OpenCV prebuilt into project..."
    Copy-Item $src $dst -Recurse -Force
    if (-not (Test-OpenCvPrebuilt -BaseRoot $ProjectRoot)) {
        throw "OpenCV prebuilt copy into project failed."
    }
}

if (-not (Test-Path $DistDir)) {
    New-Item -ItemType Directory -Force -Path $DistDir | Out-Null
}
if (Test-Path $KitDir) {
    Remove-Item -Recurse -Force $KitDir
}
New-Item -ItemType Directory -Force -Path $KitDir | Out-Null

Write-Host "Output kit folder: $KitDir"

# --- OpenCV prebuilt (required for build on another PC) ---
$hasOpenCV = $false
if (-not $SkipOpenCV) {
    Ensure-OpenCvPrebuilt
    $hasOpenCV = $true
} else {
    Write-Host "Skipping OpenCV prebuilt -SkipOpenCV."
    $hasOpenCV = Test-OpenCvPrebuilt -BaseRoot $Root
}

# --- 1. Full source code ---
$SourceDst = Join-Path $KitDir "WhereLogic"
New-Item -ItemType Directory -Force -Path $SourceDst | Out-Null
Write-Host "Copying full source code..."
Invoke-RobocopyProject -Src $Root -Dst $SourceDst
Assert-SourceTree -SourceRoot $SourceDst

if ($hasOpenCV) {
    Sync-OpenCvPrebuiltIntoProject -ProjectRoot $SourceDst
}

# --- 2. Portable game (with OpenCV DLLs if prebuilt exists) ---
$hasPortable = $false
if (-not $SkipPortable) {
    $PortableScript = Join-Path $Root "scripts\package_portable.ps1"
    $PortableDst = Join-Path $KitDir "portable"
    if (Test-Path $PortableScript) {
        Write-Host "Building portable game folder..."
        try {
            & $PortableScript -OutDir $PortableDir
            if (Test-Path (Join-Path $PortableDir "WhereLogicGame.exe")) {
                if (Test-Path $PortableDst) {
                    Remove-Item -Recurse -Force $PortableDst
                }
                Copy-Item $PortableDir $PortableDst -Recurse -Force
                $hasPortable = $true
            }
        } catch {
            Write-Warning "Portable build failed: $_"
            if (Test-Path (Join-Path $PortableDir "WhereLogicGame.exe")) {
                Copy-Item $PortableDir $PortableDst -Recurse -Force
                $hasPortable = $true
            }
        }
    } else {
        Write-Warning "package_portable.ps1 not found - portable/ omitted."
    }
} else {
    Write-Host "Skipping portable/ -SkipPortable."
}

# --- 3. User data ---
$hasUserData = $false
if (-not $SkipUserData) {
    $AppDataSrc = Join-Path $env:APPDATA "WhereLogic"
    if (Test-Path $AppDataSrc) {
        Write-Host "Copying user data from: $AppDataSrc"
        Copy-Item $AppDataSrc (Join-Path $KitDir "userdata") -Recurse -Force
        $hasUserData = $true
    } else {
        Write-Warning "AppData not found: $AppDataSrc"
    }
} else {
    Write-Host "Skipping userdata/ -SkipUserData."
}

# --- README (inside kit) ---
$readmeRu = @"
WhereLogic - локальный комплект ($Stamp)
========================================

Содержимое:
  WhereLogic/  - полный исходный код
  portable/    - готовая игра (если была Release-сборка)
  userdata/    - ваши загадки и настройки

См. также файл КАК-РАСПАКОВАТЬ.txt рядом с ZIP-архивом в папке dist.
"@
Set-Content -Path (Join-Path $KitDir "README-BUNDLE.txt") -Value $readmeRu -Encoding UTF8

# --- Manifest ---
$manifest = [ordered]@{
    formatVersion = 1
    created       = (Get-Date).ToString("o")
    machine       = $env:COMPUTERNAME
    hasSourceCode = $true
    hasOpenCV     = $hasOpenCV
    hasPortable   = $hasPortable
    hasUserData   = $hasUserData
}
$manifest | ConvertTo-Json | Set-Content (Join-Path $KitDir "manifest.json") -Encoding UTF8

# --- ZIP (optional) ---
$zipPath = ""
if (-not $SkipZip) {
    if (-not $Output) {
        $zipPath = Join-Path $DistDir "$BundleName.zip"
    } elseif ($Output -match '[\\/]$' -or -not $Output.ToLower().EndsWith('.zip')) {
        $zipPath = Join-Path $Output.TrimEnd('\', '/') "$BundleName.zip"
    } else {
        $zipPath = $Output
    }
    $zipParent = Split-Path $zipPath -Parent
    if ($zipParent -and $zipParent -notmatch '^[A-Za-z]:\\?$') {
        if (-not (Test-Path $zipParent)) {
            New-Item -ItemType Directory -Force -Path $zipParent | Out-Null
        }
    }
    if (Test-Path $zipPath) {
        Remove-Item -Force $zipPath
    }
    Write-Host "Creating archive: $zipPath"
    Compress-Archive -Path (Join-Path $KitDir "*") -DestinationPath $zipPath -CompressionLevel Optimal
}

function Write-RussianRestoreGuide {
    param(
        [string]$GuidePath,
        [string]$ArchivePath,
        [bool]$OpenCV,
        [bool]$Portable,
        [bool]$UserData
    )

    $archiveName = if ($ArchivePath) { Split-Path $ArchivePath -Leaf } else { "WhereLogic-bundle-$Stamp.zip" }
    $guideFileName = Split-Path $GuidePath -Leaf

    $openCvLine = if ($OpenCV) {
        @'
  - OpenCV prebuilt  в WhereLogic/external/WhereLogicOpenCV/prebuilt/
'@.TrimEnd()
    } else {
        @'
  - OpenCV           нет (сборка с CONFIG+=no_opencv)
'@.TrimEnd()
    }
    $portableLine = if ($Portable) {
        @'
  - portable/        готовая игра, Qt ставить не нужно
'@.TrimEnd()
    } else {
        @'
  - portable/        нет (сначала соберите Release на старом ПК)
'@.TrimEnd()
    }
    $userDataLine = if ($UserData) {
        @'
  - userdata/        ваши загадки и настройки
'@.TrimEnd()
    } else {
        @'
  - userdata/        нет
'@.TrimEnd()
    }
    $playAfterUnpack = if ($Portable) {
        @'
   Играть:  D:\WhereLogic\run-portable\WhereLogicGame.exe
'@.TrimEnd()
    } else {
        @'
   Играть:  сначала соберите Release в Qt Creator
'@.TrimEnd()
    }
    $playFromKit = if ($Portable) {
        @'
   Играть:  <путь>\WhereLogic-kit\portable\WhereLogicGame.exe
'@.TrimEnd()
    } else {
        @'
   Играть:  соберите Release в Qt Creator
'@.TrimEnd()
    }
    $userDataBlock = if ($UserData) {
        @'
   Загадки: скопируйте <путь>\WhereLogic-kit\userdata\
            в %AppData%\WhereLogic\
            (или согласитесь при запуске unpack_bundle.ps1)
'@
    } else {
        @'
   Загадки: не включены в этот комплект
'@.TrimEnd()
    }

    $body = @'
WhereLogic - как восстановить на другом ПК
========================================
Дата комплекта: {STAMP}
Создано на: {COMPUTERNAME}

Что внутри:
  - WhereLogic/     исходный код (game, scripts, config, tests...)
{OPEN_CV_LINE}
{PORTABLE_LINE}
{USERDATA_LINE}

Не копируется (это нормально): папки build/, build-ci/, dist/ с машины сборки.
Qt на другом ПК нужен только если хотите ПЕРЕСОБИРАТЬ проект.

========================================
ВАРИАНТ А - у вас ZIP-архив
========================================

1. Скопируйте на другой ПК три файла из папки dist\:
     {ARCHIVE_NAME}
     unpack_bundle.ps1
     этот файл ({GUIDE_FILE_NAME})

2. Откройте PowerShell в папке с этими файлами и выполните:

   powershell -ExecutionPolicy Bypass -File .\unpack_bundle.ps1 `
     -Archive ".\{ARCHIVE_NAME}" `
     -Target "D:\WhereLogic"

   Скрипт спросит, восстанавливать ли загадки в AppData - ответьте Y.

3. После распаковки:
{PLAY_AFTER_UNPACK}
   Код:     откройте D:\WhereLogic\WhereLogic.pro в Qt Creator

========================================
ВАРИАНТ Б - скопировали папку WhereLogic-kit целиком
========================================

Распаковка не нужна. Папка kit уже готова:

{PLAY_FROM_KIT}
   Код:     <путь>\WhereLogic-kit\WhereLogic\WhereLogic.pro

{USERDATA_BLOCK}

========================================
Сборка из исходников (Qt 6.5+ MinGW, модуль HttpServer)
========================================

   cd D:\WhereLogic
   mkdir build
   cd build
   qmake ..\WhereLogic.pro CONFIG+=release
   mingw32-make -j4

Готово.
'@

    $body = $body.Replace('{STAMP}', $Stamp)
    $body = $body.Replace('{COMPUTERNAME}', $env:COMPUTERNAME)
    $body = $body.Replace('{ARCHIVE_NAME}', $archiveName)
    $body = $body.Replace('{GUIDE_FILE_NAME}', $guideFileName)
    $body = $body.Replace('{OPEN_CV_LINE}', $openCvLine)
    $body = $body.Replace('{PORTABLE_LINE}', $portableLine)
    $body = $body.Replace('{USERDATA_LINE}', $userDataLine)
    $body = $body.Replace('{PLAY_AFTER_UNPACK}', $playAfterUnpack)
    $body = $body.Replace('{PLAY_FROM_KIT}', $playFromKit)
    $body = $body.Replace('{USERDATA_BLOCK}', $userDataBlock.TrimEnd())

    Set-Content -Path $GuidePath -Value $body -Encoding UTF8
    Write-Host "Instructions:  $GuidePath"
}

$guideBase = if ($zipPath) {
    [System.IO.Path]::ChangeExtension($zipPath, $null).TrimEnd('.')
} else {
    Join-Path $DistDir $BundleName
}
$guidePath = "$guideBase-КАК-РАСПАКОВАТЬ.txt"
Write-RussianRestoreGuide `
    -GuidePath $guidePath `
    -ArchivePath $zipPath `
    -OpenCV $hasOpenCV `
    -Portable $hasPortable `
    -UserData $hasUserData

# unpack script next to ZIP (run on other PC without opening archive first)
$unpackBesideZip = Join-Path $DistDir "unpack_bundle.ps1"
Copy-Item (Join-Path $Root "scripts\unpack_bundle.ps1") $unpackBesideZip -Force

Write-Host ""
Write-Host "Done."
Write-Host "  Source code: $(Join-Path $KitDir 'WhereLogic')"
Write-Host "  Game folder: $PortableDir"
if ($zipPath) {
    Write-Host "  Archive:     $zipPath"
}
Write-Host "  Guide:       $guidePath"
Write-Host "  Unpack tool: $unpackBesideZip"
Write-Host "  OpenCV:      $(if ($hasOpenCV) { 'yes' } else { 'no' })"
Write-Host "  Portable:    $(if ($hasPortable) { 'yes' } else { 'no - build Release and re-run' })"
Write-Host "  User data:   $(if ($hasUserData) { 'yes' } else { 'no' })"
Write-Host ""
Write-Host "Copy to flash drive from dist\\: ZIP + unpack_bundle.ps1 + *-КАК-РАСПАКОВАТЬ.txt"
