#!/usr/bin/env pwsh
# Build script for Nakama SDK with WebSocket and HTTP support using libHttpClient
# This script builds the Nakama SDK from source with WebSocket capabilities enabled

Write-Host "Building Nakama C++ SDK with WebSocket support..." -ForegroundColor Green
Write-Host ""

# Get script directory and change to nakama-sdk
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location "$scriptDir\nakama-sdk"
if (-not (Test-Path ".")) {
    Write-Host "Error: Could not navigate to nakama-sdk directory at $scriptDir\nakama-sdk" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host "Step 1: Configuring CMake with WebSocket and HTTP support..." -ForegroundColor Yellow
& cmake -S . -B build-with-ws `
    -DCMAKE_TOOLCHAIN_FILE="submodules/vcpkg/scripts/buildsystems/vcpkg.cmake" `
    -G "Visual Studio 17 2022" `
    -A x64 `
    -DWITH_WS_LIBHTTPC=ON `
    -DWITH_HTTP_LIBHTTPC=ON

if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: CMake configuration failed" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host ""
Write-Host "Step 2: Building Debug configuration..." -ForegroundColor Yellow
& cmake --build build-with-ws --config Debug

if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Debug build failed" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host ""
Write-Host "Step 3: Building MinSizeRel configuration (libHttpClient equivalent of Release)..." -ForegroundColor Yellow
& cmake --build build-with-ws --config MinSizeRel

if ($LASTEXITCODE -ne 0) {
    Write-Host "Warning: MinSizeRel build failed, continuing with Debug only..." -ForegroundColor Yellow
    $releaseBuilt = $false
} else {
    $releaseBuilt = $true
}

Write-Host ""
Write-Host "Step 4: Copying built libraries to project..." -ForegroundColor Yellow
Set-Location $scriptDir

# Copy Debug files
Write-Host "Copying Debug files..." -ForegroundColor Cyan
Copy-Item "nakama-sdk\build-with-ws\Debug\nakama-sdk.dll" "Mods\NakamaX4Client\cpp\third-party\nakama-sdk\win-x64\lib\" -Force
Copy-Item "nakama-sdk\build-with-ws\Debug\nakama-sdk.lib" "Mods\NakamaX4Client\cpp\third-party\nakama-sdk\win-x64\lib\" -Force

# Copy MinSizeRel files (equivalent to Release)
if ($releaseBuilt) {
    Write-Host "Copying MinSizeRel files..." -ForegroundColor Cyan
    Copy-Item "nakama-sdk\build-with-ws\MinSizeRel\nakama-sdk.dll" "Mods\NakamaX4Client\cpp\third-party\nakama-sdk\win-x64\lib\nakama-sdk-release.dll" -Force
    Copy-Item "nakama-sdk\build-with-ws\MinSizeRel\nakama-sdk.lib" "Mods\NakamaX4Client\cpp\third-party\nakama-sdk\win-x64\lib\nakama-sdk-release.lib" -Force
} else {
    Write-Host "Skipping MinSizeRel files (build failed)..." -ForegroundColor Yellow
}

# Copy updated headers and config
Write-Host "Copying headers and config..." -ForegroundColor Cyan
Copy-Item "nakama-sdk\interface\include\*" "Mods\NakamaX4Client\cpp\third-party\nakama-sdk\win-x64\include\" -Recurse -Force
Copy-Item "nakama-sdk\build-with-ws\interface\nakama-cpp\config.h" "Mods\NakamaX4Client\cpp\third-party\nakama-sdk\win-x64\include\nakama-cpp\" -Force

# Copy dependency DLLs to build directory
Write-Host "Copying dependency DLLs..." -ForegroundColor Cyan
Copy-Item "nakama-sdk\build-with-ws\Debug\*.dll" "build\Debug\" -Force
if ($releaseBuilt) {
    Copy-Item "nakama-sdk\build-with-ws\MinSizeRel\*.dll" "build\Release\" -Force
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "Nakama SDK build completed successfully!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Built with features:" -ForegroundColor White
Write-Host "- WebSocket support (libHttpClient)" -ForegroundColor White
Write-Host "- HTTP support (libHttpClient)" -ForegroundColor White
Write-Host "- Debug and MinSizeRel configurations" -ForegroundColor White
Write-Host ""
Write-Host "Files copied to:" -ForegroundColor White
Write-Host "- Libraries: Mods\NakamaX4Client\cpp\third-party\nakama-sdk\win-x64\lib\" -ForegroundColor White
Write-Host "- Headers: Mods\NakamaX4Client\cpp\third-party\nakama-sdk\win-x64\include\" -ForegroundColor White
Write-Host "- Dependencies: build\Debug\ and build\Release\" -ForegroundColor White
Write-Host ""
Read-Host "Press Enter to exit"