# Build script for HenMods
# This script configures and builds the C++ projects

param(
    [string]$BuildType = "Debug",
    [switch]$Clean,
    [switch]$Test,
    [switch]$CopyFiles = $False,
    [switch]$RegenerateBindings = $False,
    [string]$Mod = "all",  # "all", "commons", "nakama"
    [switch]$NoBuild,
    [switch]$Help
)

# Show help if requested or if -? is used
if ($Help -or $args -contains "-?") {
    Write-Host "HenMods Build Script" -ForegroundColor Green
    Write-Host "====================" -ForegroundColor Green
    Write-Host ""
    Write-Host "USAGE:" -ForegroundColor Yellow
    Write-Host "    .\build_mods.ps1 [options]"
    Write-Host ""
    Write-Host "OPTIONS:" -ForegroundColor Yellow
    Write-Host "    -BuildType <type>        Build configuration: Debug (default) or Release"
    Write-Host "    -Mod <mod>               Module to build: all (default), commons, or nakama"
    Write-Host "    -Clean                   Clean build directory before building"
    Write-Host "    -Test                    Run unit tests after building"
    Write-Host "    -CopyFiles               Copy built DLLs to mod directories"
    Write-Host "    -RegenerateBindings      Force regeneration of Lua bindings (works with -NoBuild)"
    Write-Host "    -NoBuild                 Skip CMake configuration and building (for testing/regeneration only)"
    Write-Host "    -Help, -?                Show this help message"
    Write-Host ""
    Write-Host "EXAMPLES:" -ForegroundColor Yellow
    Write-Host "    .\build_mods.ps1 -Mod nakama -BuildType Release -Test"
    Write-Host "    .\build_mods.ps1 -Clean -CopyFiles"
    Write-Host "    .\build_mods.ps1 -NoBuild -Test"
    Write-Host "    .\build_mods.ps1 -NoBuild -RegenerateBindings"
    Write-Host "    .\build_mods.ps1 -Help"
    Write-Host ""
    exit 0
}

$ErrorActionPreference = "Stop"

# Set paths
$ProjectRoot = $PSScriptRoot
$ModsDir = Join-Path $ProjectRoot "Mods"
$BuildDir = Join-Path $ProjectRoot "build"

if (!$NoBuild) {
    Write-Host "Building HenMods ($BuildType)" -ForegroundColor Green
} else {
    Write-Host "HenMods Script ($BuildType) - No Build Mode" -ForegroundColor Green
}
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

# Configure and build with CMake (skip if -NoBuild is specified and -RegenerateBindings is not)
if (!$NoBuild -or $RegenerateBindings) {
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

    # Only build if not skipping build entirely
    if (!$NoBuild) {
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
    } else {
        Write-Host "CMake configuration completed (bindings regenerated)" -ForegroundColor Green
        Write-Host "Skipping build (-NoBuild specified)" -ForegroundColor Yellow
    }
} else {
    Write-Host "Skipping CMake configuration and build (-NoBuild specified)" -ForegroundColor Yellow
}

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