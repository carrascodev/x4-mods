--[[
Sector Manager for Nakama X4 Client
Handles creation and management of remote player ships in sectors based on SectorMatchManager data.

This script periodically checks for players in the current sector and creates/destroys ships accordingly.
]]

local Lib = require("extensions.sn_mod_support_apis.ui.Library")
local nakamaLib = require("extensions.NakamaX4Client.lua.nakama_lib")
local logLib = require("extensions.NakamaX4Client.lua.log_lib")



-- Local state
local L = {
    initialized = false,
    last_update = 0,
    update_interval = 1.0,  -- Update every second
    created_ships = {},     -- Map of player_id -> ship object
    sector_match_manager = nil,
}
local sectorManager = nil
local function Load_Dll()
    local lib, err = package.loadlib(
        "D:\\Games\\X4 Foundations\\extensions\\NakamaX4Client\\ui\\nakama\\nakama_x4.dll", 
        "luaopen_sector_match")
    
    if lib then
        DebugError("[Nakama] DLL loaded successfully, calling luaopen_sector_match")
        return lib()  -- Call the function to get the table of functions
    else
        DebugError("[Nakama] DLL load failed: " .. tostring(err))
        return nil
    end
end

-- Check if this is running on Windows.
-- First character in package.config is the separator, which
-- is backslash on windows.
-- Note: dont try to load in ui protected mode.
if package ~= nil and package.config:sub(1,1) == "\\" and GetUISafeModeOption() == false then
    local success, value = pcall(Load_Dll, "")
    DebugError("[SectorManager] DLL load attempt result: "..tostring(value))
    if success then
        sectorManager = value
    end
end

local function LogError(msg)
    if logLib and logLib.Log then
        logLib.Log("[ERROR] " .. msg)
    else
        DebugError("[ERROR] " .. msg)
    end
end

local function LogInfo(msg)
    if logLib and logLib.Log then
        logLib.Log("[INFO] " .. msg)
    else
        DebugError("[INFO] " .. msg)
    end
end

-- Signal to MD to create a ship for a remote player
function L.CreateShipForPlayer(playerId, shipData)
    if not playerId or playerId == "" then
        LogError("Invalid player ID")
        return false
    end

    -- Skip local player
    if playerId == GetPlayerID() then
        return false
    end

    -- Check if ship already exists
    if L.created_ships[playerId] then
        return true
    end

    -- Trigger MD event to create the ship
    AddUiTriggeredEvent('Nakama', 'create_ship', {player_id = playerId, position = shipData.position})

    -- Assume success for now, mark as created
    L.created_ships[playerId] = true
    LogInfo("Triggered MD to create ship for player " .. playerId)
    return true
end

-- Update ship position for a player (placeholder, since ships are created by MD)
function L.UpdateShipPosition(playerId, position)
    -- Since ships are managed by MD, we might not need to update positions here
    -- MD can handle movement based on network data
    LogInfo("Position update for player " .. playerId .. " (handled by MD)")
end

-- Main update function
function L.Update()
    local current_time = GetCurrentTime()
    if current_time - L.last_update < L.update_interval then
        return
    end
    L.last_update = current_time

    local manager = sectorManager
    if not manager then
        return
    end

    -- Get players in sector
    local players_in_sector = manager.GetPlayersInSector
    if not players_in_sector then
        LogError("GetPlayersInSector not available")
        return
    end

    -- Iterate over players
    for player_id, ship_data in pairs(players_in_sector()) do
        if not L.created_ships[player_id] then
            L.CreateShipForPlayer(player_id, ship_data)
        end
        -- Update position if needed
        L.UpdateShipPosition(player_id, ship_data.position)
    end

    -- TODO: Handle players leaving sector (cleanup)
    
end

-- Initialize
function L.Initialize()
    if L.initialized then
        return
    end

    LogInfo("Initializing Sector Manager")
    L.initialized = true
    L.last_update = GetCurrentTime()

    -- Register for ship created event from MD
    RegisterEvent("Nakama.ShipCreated", function(_, playerId)
        LogInfo("Ship created for player " .. playerId)
    end)
    
    -- Call C++ to register our Lua update callback
    local manager = sectorManager
    if manager and manager.RegisterLuaUpdateCallback then
        manager:RegisterLuaUpdateCallback(L.Update)
        LogInfo("Registered Lua update callback with SectorMatchManager")
    else
        LogWarning("Could not register update callback - will need manual Update() calls")
    end
end

-- Cleanup
function L.Shutdown()
    LogInfo("Shutting down Sector Manager")
    L.created_ships = {}
    L.sector_match_manager = nil
    L.initialized = false
end

-- Initialize on load
L.Initialize()

Register_Require_With_Init("extensions.NakamaX4Client.lua.sector_manager", L.Initialize)

return L
