# WhereLogic — загрузка опциональных зависимостей (Windows)
# Запуск из корня репозитория:
#   powershell -ExecutionPolicy Bypass -File scripts\fetch_deps.ps1
#   powershell -ExecutionPolicy Bypass -File scripts\fetch_deps.ps1 -Whisper -OpenCV

param(
    [switch]$Whisper,
    [switch]$OpenCV,
    [switch]$ModelOnly
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $Root

function Write-Step($msg) { Write-Host "`n==> $msg" -ForegroundColor Cyan }

# --- Whisper.cpp (исходники) ---
if ($Whisper -or $ModelOnly) {
    Write-Step "Whisper.cpp submodule"
    $wp = Join-Path $Root "third_party\whisper.cpp"
    if (-not (Test-Path $wp)) {
        git submodule add https://github.com/ggerganov/whisper.cpp third_party/whisper.cpp 2>$null
        git submodule update --init third_party/whisper.cpp
    }
    if (-not (Test-Path $wp)) {
        git clone --depth 1 https://github.com/ggerganov/whisper.cpp $wp
    }
}

# --- Модель ggml-tiny ---
if ($Whisper -or $ModelOnly) {
    Write-Step "Whisper model ggml-tiny.bin (~75 MB)"
    $modelDir = Join-Path $Root "build\models"
    New-Item -ItemType Directory -Force -Path $modelDir | Out-Null
    $modelFile = Join-Path $modelDir "ggml-tiny.bin"
    if (-not (Test-Path $modelFile)) {
        $url = "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.bin"
        Invoke-WebRequest -Uri $url -OutFile $modelFile
        Write-Host "Saved: $modelFile"
    } else {
        Write-Host "Already exists: $modelFile"
    }
}

# --- OpenCV prebuilt (официальный пакет MSVC) ---
if ($OpenCV) {
    Write-Warning @"
OpenCV official Windows build = MSVC (vc16).
Your kit is MinGW — these libs link only with MSVC kit in Qt Creator.
For MinGW skip -OpenCV or use MSVC kit, or build OpenCV for MinGW yourself.
"@
    Write-Step "OpenCV 4.10.0 Windows (MSVC)"
    $ver = "4.10.0"
    $zip = Join-Path $env:TEMP "opencv-$ver-windows.exe"
    $url = "https://github.com/opencv/opencv/releases/download/$ver/opencv-$ver-windows.exe"
    if (-not (Test-Path $zip)) {
        Invoke-WebRequest -Uri $url -OutFile $zip
    }
    $extractDir = Join-Path $env:TEMP "opencv-$ver"
    if (-not (Test-Path $extractDir)) {
        Write-Host "Extracting (7-Zip self-extractor)..."
        Start-Process -FilePath $zip -ArgumentList "-o$extractDir", "-y" -Wait
    }
    $prebuilt = Join-Path $Root "external\WhereLogicOpenCV\prebuilt"
    $srcInclude = Join-Path $extractDir "opencv\build\include"
    $srcLib = Join-Path $extractDir "opencv\build\x64\vc16\lib"
    $srcBin = Join-Path $extractDir "opencv\build\x64\vc16\bin"
    New-Item -ItemType Directory -Force -Path "$prebuilt\include" | Out-Null
    New-Item -ItemType Directory -Force -Path "$prebuilt\lib" | Out-Null
    New-Item -ItemType Directory -Force -Path "$prebuilt\bin" | Out-Null
    Copy-Item -Recurse -Force "$srcInclude\*" "$prebuilt\include\"
    Copy-Item -Force "$srcLib\*.lib" "$prebuilt\lib\"
    Copy-Item -Force "$srcBin\opencv_world*.dll" "$prebuilt\bin\"
    Write-Host "OpenCV prebuilt -> $prebuilt"
    Write-Host "Use Qt kit: Desktop Qt MSVC 64-bit (not MinGW) for OpenCV."
}

Write-Step "Done"
Write-Host @"

Next steps:
1. Qt HttpServer — ONLY via Qt Maintenance Tool (cannot script):
   Maintenance Tool -> Qt 6.11.1 -> MinGW 64-bit -> Qt HttpServer -> Apply
   Then: Build -> Run qmake -> Rebuild All

2. Whisper: remove CONFIG+=no_whisper from game/WhereLogicGame.pro after submodule exists,
   rebuild (full whisper.cpp link still TODO in whisper.pri stub).

3. OpenCV: MSVC kit + remove CONFIG+=no_opencv, copy DLLs to exe folder on run.

Game and Presenter work WITHOUT whisper/opencv. HttpServer needed for phone remote.
"@
