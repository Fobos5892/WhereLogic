param(
    [string]$BuildType = "Release"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$whisperRoot = Join-Path $root "third_party/whisper.cpp"
$buildDir = Join-Path $root "build/whisper-vulkan"
$outBin = Join-Path $buildDir "bin"
$destDir = Join-Path $root "third_party/bin"

if (-not (Test-Path (Join-Path $whisperRoot "CMakeLists.txt"))) {
    throw "whisper.cpp not found. Run: git submodule update --init third_party/whisper.cpp"
}

if (-not $env:VULKAN_SDK) {
    throw "VULKAN_SDK is not set. Install Vulkan SDK and restart the shell."
}

$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    throw "cmake not found in PATH."
}

$mingwGpp = Get-Command g++ -ErrorAction SilentlyContinue
if (-not $mingwGpp) {
    throw "g++ not found in PATH. Open Qt MinGW kit environment first."
}

New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
New-Item -ItemType Directory -Path $destDir -Force | Out-Null

Push-Location $buildDir
try {
    cmake $whisperRoot `
        -G "MinGW Makefiles" `
        -DCMAKE_BUILD_TYPE=$BuildType `
        -DCMAKE_C_COMPILER=gcc `
        -DCMAKE_CXX_COMPILER=g++ `
        -DBUILD_SHARED_LIBS=ON `
        -DGGML_VULKAN=ON `
        -DWHISPER_BUILD_TESTS=OFF `
        -DWHISPER_BUILD_EXAMPLES=OFF `
        -DWHISPER_BUILD_SERVER=OFF

    cmake --build . --target ggml-vulkan -j

    $dll = Join-Path $outBin "ggml-vulkan.dll"
    if (-not (Test-Path $dll)) {
        throw "Build finished but ggml-vulkan.dll was not produced at $dll"
    }

    Copy-Item -Path $dll -Destination (Join-Path $destDir "ggml-vulkan.dll") -Force
    Write-Host "Vulkan backend ready: $destDir\ggml-vulkan.dll"
    Write-Host "Rebuild WhereLogicGame — POST_LINK copies the DLL next to the exe."
}
finally {
    Pop-Location
}
