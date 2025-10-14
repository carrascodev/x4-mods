# Build script for NakamaX4Client
# This script configures and builds the C++ project

param(
    [string]$BuildType = "Debug",
    [switch]$Clean,
    [switch]$Test,
    [switch]$CopyFiles = $False,
    [switch]$RegenerateBindings = $False
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
if ($RegenerateBindings) {
    Write-Host "Regenerate Lua bindings: YES" -ForegroundColor Cyan
} else {
    Write-Host "Regenerate Lua bindings: NO (use -RegenerateBindings to force)" -ForegroundColor DarkGray
}
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
    
    # Build CMake command with optional regeneration flag
    $cmakeArgs = @("-S", $CppDir, "-B", ".")
    if ($RegenerateBindings) {
        $cmakeArgs += "-DREGENERATE_LUA_WRAPPERS=ON"
    }
    
    & cmake @cmakeArgs
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
    Write-Host "Running tests..." -ForegroundColor Yellow
    $DebugExe = Join-Path $BuildDir "$BuildType\tests.exe"
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

if ($CopyFiles -eq $True) {
    Write-Host "Copying built DLL and PDB to project directory..." -ForegroundColor Magenta
    Copy-Item -Path (Join-Path $BuildDir "$BuildType\nakama_x4.dll") -Destination (Join-Path $ProjectRoot "Mods\NakamaX4Client\ui\nakama\nakama_x4.dll") -Force
    Copy-Item -Path (Join-Path $BuildDir "$BuildType\nakama_x4.pdb") -Destination (Join-Path $ProjectRoot "Mods\NakamaX4Client\ui\nakama\nakama_x4.pdb") -Force
    Write-Host "Files copied successfully at path: $ProjectRoot\Mods\NakamaX4Client\ui\nakama\" -ForegroundColor Green
}