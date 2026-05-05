# Copyright (c) 2026, The Arqma Network
# Configure and build `unit_tests` with BUILD_TESTS=ON (Pulse GTest lives in tests/unit_tests/pulse_round.cpp).
# Requires CMake + a working toolchain (MSVC, Ninja+GCC, etc.) on PATH.
param(
    [string]$BuildDir = "build-tests-pulse",
    [switch]$Run,
    [string]$GTestFilter = "Pulse*"
)

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Out = Join-Path $RepoRoot $BuildDir

Write-Host "Source: $RepoRoot"
Write-Host "Build:  $Out"

& cmake -S $RepoRoot -B $Out -D BUILD_TESTS=ON
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& cmake --build $Out --target unit_tests --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($Run) {
    $candidates = @(
        (Join-Path $Out "tests/unit_tests/unit_tests.exe"),
        (Join-Path $Out "tests/unit_tests/Debug/unit_tests.exe"),
        (Join-Path $Out "tests/unit_tests/Release/unit_tests.exe"),
        (Join-Path $Out "tests/unit_tests.exe"),
        (Join-Path $Out "bin/unit_tests.exe")
    )
    $exe = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $exe) {
        $found = Get-ChildItem -Path $Out -Recurse -Filter "unit_tests.exe" -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
        if ($found) { $exe = $found }
    }
    if (-not $exe) {
        Write-Error "unit_tests executable not found under $Out"
        exit 1
    }
    Write-Host "Running: $exe --gtest_filter=$GTestFilter"
    & $exe "--gtest_filter=$GTestFilter"
    exit $LASTEXITCODE
}

Write-Host "Done. Re-run with -Run to execute: --gtest_filter=$GTestFilter"
