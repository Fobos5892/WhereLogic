param(
    [Parameter(Mandatory = $true)]
    [string]$DestDir
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$candidates = @(
    (Join-Path $root "build/models"),
    (Join-Path $root "game/models"),
    (Join-Path $root "models")
)

New-Item -ItemType Directory -Path $DestDir -Force | Out-Null

foreach ($src in $candidates) {
    if (-not (Test-Path $src)) {
        continue
    }
    $model = Get-ChildItem -Path $src -Filter "ggml-*.bin" -File -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -ne $model) {
        Copy-Item -Path $model.FullName -Destination (Join-Path $DestDir $model.Name) -Force
        Write-Host "Copied Whisper model to ${DestDir}: $($model.Name)"
        break
    }
}

$exeDir = Split-Path -Parent $DestDir
$vulkanCandidates = @(
    (Join-Path $root "third_party/bin/ggml-vulkan.dll"),
    (Join-Path $root "build/whisper-vulkan/bin/ggml-vulkan.dll")
)
foreach ($dll in $vulkanCandidates) {
    if (Test-Path $dll) {
        Copy-Item -Path $dll -Destination (Join-Path $exeDir "ggml-vulkan.dll") -Force
        Write-Host "Copied Vulkan backend to ${exeDir}: ggml-vulkan.dll"
        break
    }
}

if (-not (Get-ChildItem -Path $DestDir -Filter "ggml-*.bin" -File -ErrorAction SilentlyContinue)) {
    Write-Host "Whisper model not found, skip copy."
}
exit 0
