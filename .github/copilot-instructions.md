# Copilot Instructions for X4 Nakama Mod

## Overview
This is a mod for X4: Foundations that integrates Nakama server for multiplayer features. It uses a C++ DLL to expose Nakama client functionality to Lua scripts in X4.

## Architecture
- **C++ DLL (`nakama_x4.dll`)**: Provides Lua bindings for Nakama client operations. Built with CMake using vcpkg for dependencies (nlohmann_json, msgpack-cxx).
- **Code Generation**: Lua bindings auto-generated from C++ headers in `public/` using `generate_lua_bindings.py`.
- **Lua Scripts**: Interact with the DLL to handle game events, player data, and networking.
- **UI Integration**: Mod UI in `ui.xml`, DLL placed in `Mods/NakamaX4Client/ui/nakama/`.

## Build Workflow
Use PowerShell build script for all operations:
```powershell
.\build_nakama.ps1 -BuildType Debug -Clean -Test -CopyFiles
```
- `-BuildType`: Debug (default) or Release
- `-Clean`: Remove build dir
- `-Test`: Run debug exe after build
- `-CopyFiles`: Copy DLL/PDB to mod folder

Built DLL: `build/Debug/nakama_x4.dll`

## Debug & Run
- **Launch X4**: `.\debug-x4.bat` (sets debug flags, logs to `logs/`)
- **Attach Debugger**: Use VS Code tasks "Launch X4 Debug" after starting X4
- - **Monitor Logs**: Use VS Code Debug Console or User's Documents folder "Egosoft\X4 Foundations\logs\"  to see mod logs
## Code Patterns
- **Lua Exports**: Mark C++ methods with `// LUA_EXPORT` in public headers (e.g., `nakama_x4_client.h`) for auto-generated bindings in `generated/lua_bindings.cpp` when binding is important
- **Error Handling**: Functions return `{success: bool, errorMessage: string}`; check `success` before proceeding
- **Authentication**: Blocking calls (up to 10s); use device ID as persistent unique identifier
- **Logging**: Use `LogInfo`, `LogWarning`, `LogError` macros (defined in `log_to_x4.h`)
- **Singleton Pattern**: Classes inherit `X4ScriptSingleton` for Lua integration

## Key Files
- `Mods/NakamaX4Client/cpp/src/public/`: Exported classes (NakamaX4Client, NakamaRealtimeClient, SectorMatch)
- `Mods/NakamaX4Client/lua/nakama_lib.lua`: Generated Lua bindings
- `CMakeLists.txt`: Build config with vcpkg toolchain
- `generate_lua_bindings.py`: Generates Lua bindings from C++ headers
- `log_to_x4.h`: Logging macros for the mod
- `unpacked\`: Unpacked X4 game files for reference

## Dependencies
- Nakama server running locally (default: localhost:7350)
- X4 with Mod Support APIs
- Python packages: requests, pywin32
- Vcpkg packages: nlohmann-json, msgpack-cxx

Focus on C++ for core logic and networking via Nakama, Lua for game integration.