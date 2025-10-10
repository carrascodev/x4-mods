# NakamaX4Client for X4: Foundations

A mod for X4: Foundations that integrates Nakama server features (multiplayer, leaderboards, cloud saves) via a native C++ DLL bridge, directly accessible from Lua.

## Quick Build Instructions

1. **Open PowerShell in your project root.**
2. **Run the build script:**
   ```powershell
   .\build_nakama.ps1
   ```
   - Use `-BuildType Release` for release builds (default is Debug).
   - Add `-Clean` to clean the build directory.
   - Add `-Test` to run the debug test executable after build.
   - Add `-CopyFiles` to copy the built DLL and PDB to the mod folder.

**Example:**
```powershell
.\build_nakama.ps1 -BuildType Debug -Clean -Test -CopyFiles
```
- The built DLL will be at: `build/Debug/nakama_x4.dll`
- If `-CopyFiles` is used, it will be copied to: `Mods/NakamaX4Client/ui/nakama/nakama_x4.dll`

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
- DLLs must be in `ui/nakama/`.
- Use debug build for compatibility.
- Check returned `errorMessage` for details on failures.

## Credits
- sn_mod_support_apis (bvbohnen)
- Nakama (Heroic Labs)
- X4 Community