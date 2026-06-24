# ============================================================
# run_tests.ps1
# Build and run all unit tests for MultiMediaApp.
#
# Usage (from project root, inside a Developer PowerShell with
# cl/cmake/git all resolving):
#   .\UTests\run_tests.ps1              # run all tests
#   .\UTests\run_tests.ps1 TestSQLite   # run one test suite
#   .\UTests\run_tests.ps1 --verbose    # full QtTest output
# ============================================================

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir  = Join-Path $ScriptDir "build_tests"

$Filter  = $args[0]
$Verbose = $false

if ($Filter -eq "--verbose") {
    $Verbose = $true
    $Filter  = $null
}

Write-Host "=== Building tests ==="
New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
Set-Location $BuildDir

# Reuse the same Qt path candidates as deploy_windows.ps1
$QtCandidates = @(
    "C:\Qt\6.11.1\msvc2022_arm64",
    "C:\Qt\6.11.0\msvc2022_arm64",
    "C:\Qt\6.11.1\msvc2022_64",
    "C:\Qt\6.11.0\msvc2022_64",
    "C:\Qt\6.10.2\msvc2022_64"
)
$Qt = $QtCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $Qt) {
    Write-Error "Qt installation not found. Checked: $($QtCandidates -join ', ')"
    exit 1
}

$env:QT_QPA_PLATFORM = "offscreen"

cmake $ScriptDir `
    -DCMAKE_BUILD_TYPE=Debug `
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON `
    -DCMAKE_PREFIX_PATH="$Qt"

cmake --build . --config Debug

Write-Host ""
Write-Host "=== Running tests ==="

if ($Filter) {
    $exe = Join-Path $BuildDir "$Filter.exe"
    if (-not (Test-Path $exe)) {
        $exe = Join-Path $BuildDir "Debug\$Filter.exe"
    }
    if (Test-Path $exe) {
        Write-Host "--- $Filter ---"
        & $exe -v2
    } else {
        Write-Error "Test binary '$Filter' not found under $BuildDir"
        exit 1
    }
} else {
    $ctestArgs = @("--output-on-failure")
    if ($Verbose) {
        $ctestArgs += "-V"
    }
    ctest @ctestArgs
}

Write-Host ""
Write-Host "=== Done ==="
