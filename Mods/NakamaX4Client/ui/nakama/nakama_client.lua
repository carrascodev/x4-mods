--[[
Nakama client integration for X4: Foundations
Following the sn_mod_support_apis patterns for MD-Lua integration.

This interface handles communication between MD scripts and the nakama_x4 DLL
using the same patterns as the named pipes API.
]]

-- Start debugger early (before other imports)
package.cpath = package.cpath .. ";c:/Users/henri/.vscode/extensions/tangzx.emmylua-0.9.29-win32-x64/debugger/emmy/windows/x64/?.dll"
local dbg = require("emmy_core")
dbg.tcpListen("localhost", 9966)

-- You still need ffi.C for X4 functions like GetPlayerID
local ffi = require("ffi")
ffi.cdef[[
    typedef uint64_t UniverseID;
    UniverseID GetPlayerID(void); 
]]
local C = ffi.C

-- Import lib functions and check if nakama dll is available.
local Lib = require("extensions.sn_mod_support_apis.ui.Library")
local nakama_dll = require("extensions.NakamaX4Client.lua.nakama_lib")

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
        print_to_log = false,
    },
}

-- Shared function to raise an md signal with an optional return value.
function L.Raise_Signal(name, return_value)
    -- This will give the return_value in event.param3
    -- Use <event_ui_triggered screen="'Nakama'" control="'<name>'" />
    assert(AddUITriggeredEvent ~= nil, "AddUITriggeredEvent is nil")
    AddUITriggeredEvent("Nakama", name, return_value)
    
    if L.debug.print_to_log then
        if return_value == nil then
            return_value = "nil"
        end
        DebugError("UI Event: Nakama, "..name.." ; value: "..return_value)
    end
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
function L.Init_Nakama(host, port, server_key)
    DebugError("[Nakama] Starting Init_Nakama...")
    if not nakama_dll then
        DebugError("[Nakama] ERROR: DLL not loaded")
        L.last_error = "DLL not loaded"
        return false
    end
    
    DebugError("[Nakama] DLL loaded, calling nakama_init...")
    local result = nakama_dll.nakama_init(host or "127.0.0.1", port or 7350, server_key or "defaultkey")
    DebugError("[Nakama] nakama_init returned: " .. tostring(result))
    
    if result == 1 then
        L.initialized = true
        L.last_error = ""
        DebugError("[Nakama] Init successful")
        L.Authenticate_Player(L.player_id, "Player"..tostring(L.player_id))  -- Auto-auth with defaults.
        return true
    else
        L.last_error = ffi.string(nakama_dll.nakama_get_last_error())
        DebugError("[Nakama] Init failed: " .. L.last_error)
        return false
    end
end

-- Generate a proper device ID for Nakama (must be 10-128 bytes)
-- Uses player ID to ensure uniqueness and persistence
function L.Generate_Device_ID(player_id)
    -- Convert player ID to a proper device ID format
    -- Ensure it's at least 10 characters and unique per player
    local base_id = tostring(player_id or L.player_id)
    local device_id = "x4-player-" .. base_id
    
    -- Ensure minimum length of 10 characters
    if string.len(device_id) < 10 then
        device_id = device_id .. string.rep("0", 10 - string.len(device_id))
    end
    
    DebugError("[Nakama] Generated persistent device ID: " .. device_id .. " (length: " .. string.len(device_id) .. ")")
    return device_id
end

function L.Authenticate_Player(device_id, username)
    if not nakama_dll or not L.initialized then
        L.last_error = "Not initialized"
        return false
    end
    
    -- Use persistent device ID based on player ID if not provided
    if not device_id then
        device_id = L.Generate_Device_ID(L.player_id)
    elseif string.len(device_id) < 10 then
        -- If provided device ID is too short, make it longer but still based on input
        device_id = "x4-device-" .. device_id
        DebugError("[Nakama] Extended device ID to: " .. device_id)
    end
    
    local result = nakama_dll.nakama_authenticate(device_id, username or "TestPlayer")
    if result == 1 then
        L.authenticated = true
        L.last_error = ""
        DebugError("[Nakama] Authentication successful")
        L.Raise_Signal("auth_complete", "success")
        return true
    else
        L.last_error = ffi.string(nakama_dll.nakama_get_last_error())
        DebugError("[Nakama] Authentication failed: " .. L.last_error)
        return false
    end
end

function L.Sync_Player_Data(credits, playtime)
    if not nakama_dll or not L.initialized or not L.authenticated then
        L.last_error = "Not authenticated"
        return false
    end
    
    local result = nakama_dll.nakama_sync_player_data(
        "Player"..tostring(L.player_id), 
        credits or 0, 
        playtime or 0
    )
    if result == 1 then
        L.last_error = ""
        DebugError("[Nakama] Player data sync successful")
        return true
    else
        L.last_error = ffi.string(nakama_dll.nakama_get_last_error())
        return false
    end
end

function L.Get_Status()
    if not nakama_dll then
        return "DLL not loaded"
    end
    if not L.initialized then
        return "Not initialized"
    end
    return ffi.string(nakama_dll.nakama_get_status())
end

function L.Shutdown()
    if nakama_dll and L.initialized then
        nakama_dll.nakama_shutdown()
        L.initialized = false
        L.authenticated = false
    end
end

-- Generic command handler.
-- When this is signalled, there may be multiple commands queued.
function L.Process_Command(signal, value)
    local success = false
    local result = "error"
    
    DebugError("[Nakama] Processing command: " .. signal .. " with data: " .. tostring(value))
    
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
    if nakama_dll then
        -- Generic command handler.
        RegisterEvent("Nakama.Init", L.Process_Command)
        RegisterEvent("Nakama.Auth", L.Process_Command)
        RegisterEvent("Nakama.Sync", L.Process_Command)

        --L.Init_Nakama()  -- Auto-init with defaults.
        DebugError("[Nakama] Client loaded successfully with DLL")
        L.Raise_Signal("initialize_nakama", "success")
        return true, "nakama loaded"
    else
        -- Signal to MD that nakama dll is not available.
        L.Raise_Signal('disabled', 'dll_not_found')
        DebugError("[Nakama] Client loaded but DLL not available")
    end

    return false, "nakama dll_not_found"
end

Register_Require_With_Init("extensions.NakamaX4Client.lua.nakama_client", L, Init)