# Build script for NakamaX4Client
# This script configures and builds the C++ project

param(
    [string]$BuildType = "Debug",
    [switch]$Clean,
    [switch]$Test
)

$ErrorActionPreference = "Stop"

# Set paths
$ProjectRoot = $PSScriptRoot
$CppDir = Join-Path $ProjectRoot "Mods\NakamaX4Client\cpp"
$BuildDir = Join-Path $ProjectRoot "build"

Write-Host "Building NakamaX4Client ($BuildType)" -ForegroundColor Green
Write-Host "Project root: $ProjectRoot"
Write-Host "CPP directory: $CppDir"
Write-Host "Build directory: $BuildDir"
Write-Host ""

# Clean build directory if requested
if ($Clean) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
    }
}

# Create build directory if it doesn't exist
if (!(Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# Configure with CMake
Write-Host "Configuring CMake..." -ForegroundColor Yellow
Push-Location $BuildDir
try {
    # Force clean configure by removing CMake cache
    if (Test-Path "CMakeCache.txt") {
        Remove-Item "CMakeCache.txt"
    }
    & cmake -S $CppDir -B .
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }
}
finally {
    Pop-Location
}

# Build with CMake
Write-Host "Building project ($BuildType)..." -ForegroundColor Yellow
Push-Location $BuildDir
try {
    & cmake --build . --config $BuildType
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed. Check the output for errors."
    }
}
finally {
    Pop-Location
}

Write-Host "Build completed successfully!" -ForegroundColor Green

# Run test if requested
if ($Test) {
    Write-Host "Running debug test..." -ForegroundColor Yellow
    $DebugExe = Join-Path $BuildDir "$BuildType\debug_nakama.exe"
    if (Test-Path $DebugExe) {
        & $DebugExe
    } else {
        Write-Host "Debug executable not found at: $DebugExe" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "Build completed successfully! DLL created at:" -ForegroundColor Green
$DllPath = Join-Path $BuildDir "$BuildType\nakama_x4.dll"
Write-Host $DllPath -ForegroundColor Cyan
Write-Host ""
Write-Host "You can now test the DLL in X4 or run the debug exe." -ForegroundColor Green