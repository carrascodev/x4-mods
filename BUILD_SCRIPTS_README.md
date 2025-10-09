# Nakama SDK Build Scripts

This folder contains scripts to build the Nakama C++ SDK from source with WebSocket support enabled.

## Available Scripts

### Windows
- **`build_nakama_sdk.bat`** - Windows batch script
- **`build_nakama_sdk.ps1`** - PowerShell script (recommended for Windows)

### Linux/macOS
- **`build_nakama_sdk.sh`** - Bash shell script

## What the Scripts Do

1. **Configure CMake** with the following options:
   - `WITH_WS_LIBHTTPC=ON` - Enable WebSocket support using libHttpClient
   - `WITH_HTTP_LIBHTTPC=ON` - Enable HTTP support using libHttpClient
   - Use vcpkg for dependency management
   - Target Visual Studio 2022 x64 (Windows) or default generator (Linux/macOS)

2. **Build** both Debug and MinSizeRel configurations

3. **Copy files** to the appropriate locations:
   - Built libraries to `Mods/NakamaX4Client/cpp/third-party/nakama-sdk/`
   - Updated headers with WebSocket support
   - Updated config.h with proper feature definitions
   - Dependency DLLs to build directories

## Usage

### Windows (Batch)
```cmd
build_nakama_sdk.bat
```

### Windows (PowerShell)
```powershell
.\build_nakama_sdk.ps1
```

### Linux/macOS
```bash
chmod +x build_nakama_sdk.sh
./build_nakama_sdk.sh
```

## Prerequisites

- CMake 3.29.3 or later
- Visual Studio 2022 with C++ workload (Windows)
- GCC or Clang (Linux/macOS)
- Git (for submodules)

## What Gets Built

The scripts build the Nakama SDK with:
- **WebSocket Support**: Real-time multiplayer capabilities
- **HTTP Support**: REST API functionality
- **libHttpClient**: Microsoft's HTTP and WebSocket library
- **Debug & MinSizeRel**: Both configurations for development and optimized production

## Output Files

After running the script, you'll have:
- `nakama-sdk.dll/.so` - Main Nakama SDK library
- `config.h` - Updated with WebSocket feature flags
- Dependency libraries in build directories
- Complete header files with WebSocket APIs

## Troubleshooting

**Why MinSizeRel instead of Release?**
The libHttpClient submodule only provides Debug and MinSizeRel configurations, not Release. MinSizeRel is optimized for size and performance, making it equivalent to a Release build.

If the build fails:
1. Ensure all git submodules are initialized: `git submodule update --init --recursive`
2. Check that vcpkg is properly set up in the nakama-sdk submodule
3. Verify Visual Studio 2022 is installed with C++ workload (Windows)
4. Make sure CMake is in your PATH

## Integration

Once built, the X4 mod will automatically use the WebSocket-enabled SDK when you rebuild your project with:
```cmd
cmake --build build --config Debug
```