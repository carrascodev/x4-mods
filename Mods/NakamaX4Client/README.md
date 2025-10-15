# NakamaX4Client - X4 Multiplayer Mod

A mod for X4: Foundations that integrates Nakama server features (multiplayer, leaderboards, cloud saves) via a native C++ DLL bridge, directly accessible from Lua.

This mod depends on `HenMod.Commons` for shared utilities and base classes.

## Quick Build Instructions

### Build the Entire Project
```powershell
# From project root - builds commons + NakamaX4Client
.\build_mods.ps1 -Mod all -BuildType Debug -Clean -Test -CopyFiles
```

### Build This Mod Only
```powershell
# From project root - builds only NakamaX4Client (requires commons to be built first)
.\build_mods.ps1 -Mod nakama -BuildType Debug -Test
```

**Build Output:**
- DLL: `Mods/NakamaX4Client/ui/bin/Debug/nakama_x4.dll`
- With `-CopyFiles`: Also copied to `Mods/NakamaX4Client/ui/nakama/nakama_x4.dll`

## Architecture

This mod uses a modular C++ architecture:

- **HenMod.Commons**: Shared library with base classes and utilities
- **NakamaX4Client**: Mod-specific implementation inheriting from commons
- **Lua Bindings**: Auto-generated from C++ headers marked with `// LUA_EXPORT`
- **Testing**: Catch2 for unit tests, FakeIt for mocking

## Integration: Key Lua API

All functions are available via `require("extensions.NakamaX4Client.lua.nakama_lib")`.

### Minimal Example
```lua
local nakamaLib = require("extensions.NakamaX4Client.lua.nakama_lib")
if nakamaLib then
  if nakamaLib.Initialize("localhost", 7350, "defaultkey", false) then
    local auth = nakamaLib.Authenticate("device_id", "player")
    if auth.success then
      nakamaLib.SyncPlayerData("player", 1000, 3600)
    end
  end
end
```

### Typical Usage Patterns
```lua
-- Initialize connection (returns boolean)
local ok = nakamaLib.Initialize(host, port, server_key, use_ssl)

-- Authenticate (returns table)
local result = nakamaLib.Authenticate(device_id, username)
if result.success then
  -- Authenticated!
else
  DebugError(result.errorMessage)
end

-- Sync player data (returns table)
local result = nakamaLib.SyncPlayerData(player_name, credits, playtime)
if result.success then
  -- Synced!
else
  DebugError(result.errorMessage)
end

-- Check authentication status (boolean)
local is_auth = nakamaLib.IsAuthenticated()

-- Shutdown
nakamaLib.Shutdown()
```

## Key Details
- **Initialize** returns a boolean (true/false). All other major functions return a table `{ success, errorMessage }`.
- **Device ID**: Use a persistent, unique string per player for authentication.
- **Blocking Calls**: Authentication and sync block for up to 10 seconds (not truly async).
- **DLL Loader**: Uses `package.loadlib` for Windows; ensure DLLs are in the correct folder.
- **Wrapper Generator**: Python script auto-generates Lua bindings for C++ classes marked with `// LUA_EXPORT`.

## Troubleshooting

- **Build Issues**: Ensure `HenMod.Commons` is built first with `.\build_mods.ps1 -Mod commons`
- **DLL Location**: Must be in `ui/nakama/` folder for X4 to load
- **Debug Build**: Use debug builds for better error messages and compatibility
- **Dependencies**: Check that all vcpkg packages are installed (`nlohmann-json`, `msgpack-cxx`, `catch2`, `fakeit`)
- **Error Messages**: Check returned `errorMessage` fields for detailed failure information

## Project Structure

```
Mods/NakamaX4Client/
├── cpp/src/public/          # Mod-specific C++ headers
├── cpp/src/private/         # Mod-specific C++ implementations
├── cpp/tests/               # Unit tests
├── lua/                     # Generated Lua bindings
├── ui/                      # X4 UI integration files
└── server/                  # Nakama server configuration
```

## Credits

- **Heroic Labs** - Nakama server and SDK
- **bvbohnen** - X4 mod support APIs
- **X4 Community** - Modding support and documentation
- **HenMod.Commons** - Shared C++ utilities and base classes