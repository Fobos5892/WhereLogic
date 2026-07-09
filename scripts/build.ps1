param(
    [string]$BuildDir = "",
    [int]$Jobs = 0
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

if ($Jobs -lt 1) {
    $Jobs = [int]$env:NUMBER_OF_PROCESSORS
}
if ($Jobs -lt 1) { $Jobs = 4 }

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $candidates = @(
        (Join-Path $repoRoot "build\Desktop_Qt_6_11_1_MinGW_64_bit-Debug\game"),
        (Join-Path $repoRoot "build\game")
    )
    foreach ($candidate in $candidates) {
        if (Test-Path (Join-Path $candidate "Makefile")) {
            $BuildDir = $candidate
            break
        }
    }
}

if ([string]::IsNullOrWhiteSpace($BuildDir) -or -not (Test-Path (Join-Path $BuildDir "Makefile"))) {
    throw "Build directory with Makefile not found. Pass -BuildDir <path>."
}

$make = Get-Command mingw32-make -ErrorAction SilentlyContinue
if (-not $make) {
    $qtMake = "C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe"
    if (Test-Path $qtMake) {
        $make = $qtMake
    } else {
        throw "mingw32-make not found in PATH."
    }
} else {
    $make = $make.Source
}

Write-Host "Building in: $BuildDir"
Write-Host "Parallel jobs: $Jobs"
Push-Location $BuildDir
try {
    & $make "-j$Jobs"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} finally {
    Pop-Location
}
