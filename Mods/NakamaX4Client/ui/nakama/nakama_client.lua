--[[
Nakama client integration for X4: Foundations
Following the sn_mod_support_apis patterns for MD-Lua integration.

This interface handles communication between MD scripts and the nakama_x4 DLL
using the same patterns as the named pipes API.
]]

-- You still need ffi.C for X4 functions like GetPlayerID
local ffi = require("ffi")
ffi.cdef [[
    typedef uint64_t UniverseID;
    UniverseID GetPlayerID(void);
]]
local C = ffi.C

-- Import lib functions and check if nakama dll is available.
local Lib = require("extensions.sn_mod_support_apis.ui.Library")
local nakamaLib = require("extensions.NakamaX4Client.lua.nakama_lib")
local logLib = require("extensions.NakamaX4Client.lua.log_lib")

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



-- Import UI event functions

-- Table of local functions or data.
local L = {
    -- Any command arguments not yet processed.
    queued_args = {},
    -- Status tracking
    initialized = false,
    authenticated = false,
    last_error = "",
    debug = {
        print_to_log = true,
    },
}

-- Shared function to raise an md signal with an optional return value.
function L.Raise_Signal(name, return_value)
    -- This will give the return_value in event.param3
    -- Use <event_ui_triggered screen="'Nakama'" control="'<name>'" />
    assert(AddUITriggeredEvent ~= nil, "AddUITriggeredEvent is nil")
    AddUITriggeredEvent("Nakama", name, return_value)

    if return_value == nil then
        return_value = "nil"
    end

    LogInfo("UI Event: Nakama, " .. name .. " ; value: " .. return_value)
end

-- Get args from the player blackboard, and return the next entry.
function L.Get_Next_Args()
    -- If the list of queued args is empty, grab more from md.
    if #L.queued_args == 0 then
        -- Args are attached to the player component object.
        local args_list = GetNPCBlackboard(L.player_id, "$nakama_api_args")

        if args_list then
            -- Loop over it and move entries to the queue.
            for i, v in ipairs(args_list) do
                table.insert(L.queued_args, v)
            end

            -- Clear the md var by writing nil.
            SetNPCBlackboard(L.player_id, "$nakama_api_args", nil)
        end
    end

    -- Pop the first table entry.
    local args = table.remove(L.queued_args, 1)
    return args
end

-- Nakama API wrapper functions
function L.Init_Nakama(host, port, server_key, use_ssl)
    LogInfo("[Nakama] Starting Init_Nakama...")
    if not nakamaLib then
        LogError("[Nakama] ERROR: DLL not loaded")
        L.last_error = "DLL not loaded"
        return false
    end

    DebugError("[Nakama] DLL loaded, calling nakama_init...")
    local result, error = nakamaLib.Initialize(host or "127.0.0.1", port or 7350, server_key or "defaultkey", use_ssl or false)
    DebugError("[Nakama] nakama_init returned: " .. tostring(result))

    if result then
        L.initialized = true
        L.last_error = ""
        LogInfo("[Nakama] Init successful")
        L.Authenticate_Player(L.Generate_Device_ID(L.player_id), "Player" .. tostring(L.player_id)) -- Auto-auth with proper device ID
        return true
    else
        L.last_error = error or "Unknown error"
        LogError("[Nakama] Init failed: " .. L.last_error)
        return false
    end
end

-- Generate a proper device ID for Nakama (must be 10-128 bytes)
-- Uses player ID to ensure uniqueness and persistence
function L.Generate_Device_ID(player_id)
    -- Convert player ID to a proper device ID format
    -- Ensure it's at least 10 characters and unique per player
    local base_id = tostring(player_id)
    local device_id = "x4-player-" .. base_id

    -- Ensure minimum length of 10 characters
    if string.len(device_id) < 10 then
        device_id = device_id .. string.rep("0", 10 - string.len(device_id))
    end

    LogInfo("[Nakama] Generated persistent device ID: " .. device_id .. " (length: " .. string.len(device_id) .. ")")
    return device_id
end

function L.Authenticate_Player(device_id, username)
    if not nakamaLib or not L.initialized then
        L.last_error = "Not initialized"
        return false
    end

    local result, error = nakamaLib.Authenticate(device_id, username or "TestPlayer")
    if result then
        L.authenticated = true
        L.last_error = ""
        LogInfo("[Nakama] Authentication successful")
        L.Raise_Signal("auth_complete", "success")
        return true
    else
        L.last_error = error or "Authentication failed"
        LogError("[Nakama] Authentication failed: " .. L.last_error)
        return false
    end
end

function L.Sync_Player_Data(credits, playtime)
    if not nakamaLib or not L.initialized or not L.authenticated then
        L.last_error = "Not authenticated"
        return false
    end

    local result, error = nakamaLib.SyncPlayerData(
        "Player" .. tostring(L.player_id),
        credits or 0,
        playtime or 0
    )
    if result then
        L.last_error = ""
        LogInfo("[Nakama] Player data sync successful")
        return true
    else
        L.last_error = error or "Sync failed"
        LogError("[Nakama] Player data sync failed: " .. L.last_error)
        return false
    end
end

function L.Get_Status()
    if not nakamaLib then
        return "DLL not loaded"
    end
    if not L.initialized then
        return "Not initialized"
    end
    return "unknown"
end

function L.Shutdown()
    if nakamaLib and L.initialized then
        nakamaLib.Shutdown()
        L.initialized = false
        L.authenticated = false
    end
end

-- Generic command handler.
-- When this is signalled, there may be multiple commands queued.
function L.Process_Command(signal, value)
    local success = false
    local result = "error"

    LogInfo("[Nakama] Processing command: " .. signal .. " with data: " .. tostring(value))

    if signal == "Nakama.Init" then
        success = L.Init_Nakama()
        result = success and "success" or L.last_error
    elseif signal == "Nakama.Sync" then
        success = L.Sync_Player_Data(value.credits, value.playtime)
        result = success and "success" or L.last_error
        L.Raise_Signal("sync_complete", result)
    elseif signal == "Nakama.Status" then
        result = L.Get_Status()
        L.Raise_Signal("status_response", result)
    else
        L.Raise_Signal("unknown_command", signal or "nil")
    end
end

-- Handle initial setup.
local function Init()
    -- Cache the player component id.
    L.player_id = ConvertStringTo64Bit(tostring(C.GetPlayerID()))

    -- Clear any old command args.
    SetNPCBlackboard(L.player_id, "$nakama_api_args", nil)

    -- If nakama dll loaded, listen for md commands.
    if nakamaLib then
        -- Generic command handler.
        RegisterEvent("Nakama.Init", L.Process_Command)
        RegisterEvent("Nakama.Auth", L.Process_Command)
        RegisterEvent("Nakama.Sync", L.Process_Command)

        --L.Init_Nakama()  -- Auto-init with defaults.
        LogInfo("[Nakama] Client loaded successfully with DLL")
        L.Raise_Signal("initialize_nakama", "success")
        return true, "nakama loaded"
    else
        -- Signal to MD that nakama dll is not available.
        L.Raise_Signal('disabled', 'dll_not_found')
        LogError("[Nakama] Client loaded but DLL not available")
    end

    return false, "nakama dll_not_found"
end

Register_Require_With_Init("extensions.NakamaX4Client.lua.nakama_client", L, Init)
