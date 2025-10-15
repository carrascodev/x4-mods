# Copilot Instructions for X4 Nakama Mod

## Overview
This is a mod for X4: Foundations that integrates Nakama server for multiplayer features. It uses a modular C++ architecture with shared commons library to expose Nakama client functionality to Lua scripts in X4.

## Architecture
- **Modular Design**: `HenMod.Commons` provides shared utilities, `NakamaX4Client` implements mod-specific functionality
- **C++ DLL (`nakama_x4.dll`)**: Provides Lua bindings for Nakama client operations. Built with CMake using vcpkg for dependencies (nlohmann_json, msgpack-cxx).
- **Code Generation**: Lua bindings auto-generated from C++ headers in `public/` using `generate_lua_bindings.py`.
- **Lua Scripts**: Interact with the DLL to handle game events, player data, and networking.
- **UI Integration**: Mod UI in `ui.xml`, DLL placed in `Mods/NakamaX4Client/ui/nakama/`.

## Build Workflow
Use PowerShell build script for all operations:
```powershell
.\build_mods.ps1 -Mod nakama -BuildType Debug -Clean -Test -CopyFiles
```
- `-Mod`: `all`, `commons`, or `nakama` (default: `all`)
- `-BuildType`: Debug (default) or Release
- `-Clean`: Remove build dir
- `-Test`: Build and run tests
- `-CopyFiles`: Copy DLL/PDB to mod folder

Built DLL: `Mods/NakamaX4Client/ui/bin/Debug/nakama_x4.dll`

## Debug & Run
- **Launch X4**: `.\debug-x4.bat` (sets debug flags, logs to `logs/`)
- **Attach Debugger**: Use VS Code tasks "Launch X4 Debug" after starting X4
- **Monitor Logs**: Use VS Code Debug Console or User's Documents folder "Egosoft\X4 Foundations\logs\" to see mod logs
## Code Patterns
- **Modular Architecture**: Use `HenMod.Commons` for shared utilities, keep mod-specific code in respective mod directories
- **Lua Exports**: Mark C++ methods with `// LUA_EXPORT` in public headers (e.g., `nakama_x4_client.h`) for auto-generated bindings in `generated/lua_bindings.cpp`
- **Error Handling**: Functions return `{success: bool, errorMessage: string}`; check `success` before proceeding
- **Authentication**: Blocking calls (up to 10s); use device ID as persistent unique identifier
- **Logging**: Use `LogInfo`, `LogWarning`, `LogError` macros (defined in `HenMod.Commons`)
- **Base Classes**: Inherit from `X4ScriptBase` for common functionality, `X4ScriptSingleton` for Lua integration
- **Testing**: Use Catch2 for unit tests and FakeIt for mocking dependencies
- **MSGPACK_NO_BOOST**: Always define this to avoid boost dependencies

## Key Files
- `Mods/HenMod.Commons/cpp/src/public/`: Shared base classes (`x4_script_base.h`) and utilities (`log_to_x4.h`)
- `Mods/HenMod.Commons/cpp/src/private/`: Shared implementations (`log_to_x4.cpp`)
- `Mods/NakamaX4Client/cpp/src/public/`: Mod-specific exported classes (NakamaX4Client, NakamaRealtimeClient, SectorMatch)
- `Mods/NakamaX4Client/lua/nakama_lib.lua`: Generated Lua bindings
- `CMakeLists.txt`: Top-level build config with vcpkg toolchain and mod subdirectories
- `Mods/HenMod.Commons/CMakeLists.txt`: Commons library build configuration
- `Mods/NakamaX4Client/CMakeLists.txt`: NakamaX4Client mod build configuration
- `generate_lua_bindings.py`: Generates Lua bindings from C++ headers
- `build_mods.ps1`: PowerShell build script for modular compilation
- `unpacked\`: Unpacked X4 game files for reference

## Dependencies
- Nakama server running locally (default: localhost:7350)
- X4 with Mod Support APIs
- Python packages: requests, pywin32
- Vcpkg packages: nlohmann-json, msgpack-cxx, catch2, fakeit (no boost required)

Focus on C++ for core logic and networking via Nakama, Lua for game integration.