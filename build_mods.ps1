# Build script for HenMods
# This script configures and builds the C++ projects

param(
    [string]$BuildType = "Debug",
    [switch]$Clean,
    [switch]$Test,
    [switch]$CopyFiles = $False,
    [switch]$RegenerateBindings = $False,
    [string]$Mod = "all"  # "all", "commons", "nakama"
)

$ErrorActionPreference = "Stop"

# Set paths
$ProjectRoot = $PSScriptRoot
$ModsDir = Join-Path $ProjectRoot "Mods"
$BuildDir = Join-Path $ProjectRoot "build"

Write-Host "Building HenMods ($BuildType)" -ForegroundColor Green
Write-Host "Project root: $ProjectRoot"
Write-Host "Mods directory: $ModsDir"
Write-Host "Build directory: $BuildDir"
if ($RegenerateBindings) {
    Write-Host "Regenerate Lua bindings: YES" -ForegroundColor Cyan
} else {
    Write-Host "Regenerate Lua bindings: NO (use -RegenerateBindings to force)" -ForegroundColor DarkGray
}
Write-Host "Building mod: $Mod" -ForegroundColor Cyan
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
    
    # Build CMake command
    $cmakeArgs = @("-S", $ModsDir, "-B", ".")
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
    $buildArgs = @("--build", ".", "--config", $BuildType)
    $targets = @()
    if ($Mod -ne "all") {
        if ($Mod -eq "commons") {
            $targets += "henmod_commons"
            if ($Test) {
                $targets += "commons_tests"
            }
        } elseif ($Mod -eq "nakama") {
            $targets += "nakama_x4"
            if ($Test) {
                $targets += "nakama_tests"
            }
        }
        if ($targets.Count -gt 0) {
            $buildArgs += "--target"
            $buildArgs += $targets
        }
    }
    & cmake @buildArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed. Check the output for errors."
    }
}
finally {
    Pop-Location
}

Write-Host "Build completed successfully!" -ForegroundColor Green

# Run tests if requested
if ($Test) {
    Write-Host "Running tests..." -ForegroundColor Yellow
    if ($Mod -eq "all" -or $Mod -eq "commons") {
        $CommonsTestExe = Join-Path $BuildDir "HenMod.Commons\$BuildType\commons_tests.exe"
        if (Test-Path $CommonsTestExe) {
            & $CommonsTestExe
        }
    }
    if ($Mod -eq "all" -or $Mod -eq "nakama") {
        $NakamaTestExe = Join-Path $BuildDir "NakamaX4Client\$BuildType\nakama_tests.exe"
        if (Test-Path $NakamaTestExe) {
            & $NakamaTestExe
        }
    }
}

Write-Host ""
Write-Host "Build completed successfully! DLLs created at:" -ForegroundColor Green
if ($Mod -eq "all" -or $Mod -eq "nakama") {
    $DllPath = Join-Path $ProjectRoot "Mods\NakamaX4Client\ui\bin\nakama_x4.dll"
    Write-Host $DllPath -ForegroundColor Cyan
}
Write-Host ""
Write-Host "You can now test the DLLs in X4." -ForegroundColor Green

if ($CopyFiles -eq $True) {
    Write-Host "Copying built DLLs and PDBs to mod directories..." -ForegroundColor Magenta
    if ($Mod -eq "all" -or $Mod -eq "nakama") {
        Copy-Item -Path (Join-Path $ProjectRoot "Mods\NakamaX4Client\ui\bin\nakama_x4.dll") -Destination (Join-Path $ProjectRoot "Mods\NakamaX4Client\ui\nakama\nakama_x4.dll") -Force
        Copy-Item -Path (Join-Path $ProjectRoot "Mods\NakamaX4Client\ui\bin\nakama_x4.pdb") -Destination (Join-Path $ProjectRoot "Mods\NakamaX4Client\ui\nakama\nakama_x4.pdb") -Force
    }
    Write-Host "Files copied successfully to ui/nakama/" -ForegroundColor Green
}