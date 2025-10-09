-- Nakama server Lua runtime for X4 NakamaX4Client
-- This file enables custom server-side logic for X4 multiplayer features

local nk = require("nakama")

nk.logger_info("X4 NakamaX4Client server starting...")

-- ========================================
-- UTILITY FUNCTIONS
-- ========================================

local function validate_x4_data(data)
    -- Validate required fields for X4 player data
    if not data.player_name or type(data.player_name) ~= "string" then
        return false, "Invalid player_name"
    end
    
    if not data.credits or type(data.credits) ~= "number" then
        return false, "Invalid credits"
    end
    
    if not data.playtime or type(data.playtime) ~= "number" then
        return false, "Invalid playtime"
    end
    
    return true, "Valid"
end

-- ========================================
-- PLAYER DATA MANAGEMENT
-- ========================================

-- Custom RPC for X4 player data synchronization
local function sync_x4_player_data(context, payload)
    local user_id = context.user_id
    local username = context.username
    
    nk.logger_info(string.format("sync_x4_player_data called by user %s (%s)", username, user_id))
    
    -- Parse the incoming data
    local success, data = pcall(nk.json_decode, payload)
    if not success then
        nk.logger_error("Failed to decode player data JSON: " .. tostring(data))
        return nk.json_encode({success = false, error = "Invalid JSON"})
    end
    
    -- Validate the data
    local valid, error_msg = validate_x4_data(data)
    if not valid then
        nk.logger_error("Invalid player data: " .. error_msg)
        return nk.json_encode({success = false, error = error_msg})
    end
    
    -- Add server timestamp
    data.server_timestamp = os.time()
    data.last_sync = os.date("!%Y-%m-%dT%H:%M:%SZ")
    
    -- Store the player data
    local objects = {
        {
            collection = "x4_player_data",
            key = "profile",
            user_id = user_id,
            value = data,
            permission_read = 1,
            permission_write = 1
        }
    }
    
    local success, result = pcall(nk.storage_write, objects)
    if not success then
        nk.logger_error("Failed to write player data: " .. tostring(result))
        return nk.json_encode({success = false, error = "Storage write failed"})
    end
    
    nk.logger_info(string.format("Successfully synced data for player %s: %d credits, %d playtime", 
                                data.player_name, data.credits, data.playtime))
    
    return nk.json_encode({
        success = true, 
        timestamp = data.server_timestamp,
        message = "Player data synchronized successfully"
    })
end

-- Register the RPC
nk.register_rpc(sync_x4_player_data, "sync_x4_player_data")

-- ========================================
-- LEADERBOARDS
-- ========================================

-- Create X4-specific leaderboards
local function create_x4_leaderboards()
    -- Credits leaderboard
    pcall(nk.leaderboard_create, "x4_credits", false, "desc", "best", true)
    
    -- Playtime leaderboard
    pcall(nk.leaderboard_create, "x4_playtime", false, "desc", "best", true)
    
    -- Fleet size leaderboard
    pcall(nk.leaderboard_create, "x4_fleet_size", false, "desc", "best", true)
    
    nk.logger_info("X4 leaderboards created/verified")
end

-- Submit leaderboard scores
local function submit_x4_leaderboard(context, payload)
    local user_id = context.user_id
    local username = context.username
    
    local success, data = pcall(nk.json_decode, payload)
    if not success then
        return nk.json_encode({success = false, error = "Invalid JSON"})
    end
    
    if not data.leaderboard or not data.score then
        return nk.json_encode({success = false, error = "Missing leaderboard or score"})
    end
    
    -- Submit the score
    local success, result = pcall(nk.leaderboard_record_write, 
                                  data.leaderboard, user_id, username, data.score, data.subscore or 0)
    
    if not success then
        nk.logger_error("Failed to submit leaderboard score: " .. tostring(result))
        return nk.json_encode({success = false, error = "Leaderboard submission failed"})
    end
    
    nk.logger_info(string.format("Leaderboard submission: %s scored %d on %s", 
                                username, data.score, data.leaderboard))
    
    return nk.json_encode({success = true, message = "Score submitted successfully"})
end

nk.register_rpc(submit_x4_leaderboard, "submit_x4_leaderboard")

-- ========================================
-- REAL-TIME MULTIPLAYER (MATCHES)
-- ========================================

-- X4 Multiplayer match handler
local x4_match = {}

function x4_match.match_init(context, initial_state)
    local state = {
        players = {},
        created_at = nk.time(),
        match_id = context.match_id
    }
    
    nk.logger_info("X4 match initialized: " .. context.match_id)
    return state
end

function x4_match.match_join_attempt(context, dispatcher, tick, state, presence, metadata)
    nk.logger_info(string.format("Player %s attempting to join X4 match", presence.username))
    return state, true -- Allow all joins for now
end

function x4_match.match_join(context, dispatcher, tick, state, presences)
    for _, presence in ipairs(presences) do
        state.players[presence.user_id] = {
            username = presence.username,
            session_id = presence.session_id,
            joined_at = tick
        }
        
        -- Notify other players
        local join_msg = {
            type = "player_joined",
            user_id = presence.user_id,
            username = presence.username,
            timestamp = tick
        }
        
        dispatcher.broadcast_message(1, nk.json_encode(join_msg), {presence})
        
        nk.logger_info(string.format("Player %s joined X4 match %s", presence.username, context.match_id))
    end
    
    return state
end

function x4_match.match_leave(context, dispatcher, tick, state, presences)
    for _, presence in ipairs(presences) do
        state.players[presence.user_id] = nil
        
        -- Notify remaining players
        local leave_msg = {
            type = "player_left",
            user_id = presence.user_id,
            username = presence.username,
            timestamp = tick
        }
        
        dispatcher.broadcast_message(2, nk.json_encode(leave_msg), {presence})
        
        nk.logger_info(string.format("Player %s left X4 match %s", presence.username, context.match_id))
    end
    
    return state
end

function x4_match.match_loop(context, dispatcher, tick, state, messages)
    -- Handle real-time messages between X4 clients
    for _, message in ipairs(messages) do
        local sender = message.sender
        
        -- Decode message
        local success, data = pcall(nk.json_decode, message.data)
        if success then
            -- Add sender info and timestamp
            data.sender_id = sender.user_id
            data.sender_username = sender.username
            data.timestamp = tick
            
            -- Broadcast to all other players (excluding sender)
            dispatcher.broadcast_message(message.op_code, nk.json_encode(data), {sender})
            
            -- Log significant events
            if data.type == "sector_change" or data.type == "combat_event" then
                nk.logger_info(string.format("X4 event from %s: %s", sender.username, data.type))
            end
        end
    end
    
    return state
end

function x4_match.match_terminate(context, dispatcher, tick, state, grace_seconds)
    nk.logger_info("X4 match terminated: " .. context.match_id)
    return nil
end

-- Register the match handler
nk.register_match_handler("x4_match", x4_match)

-- ========================================
-- MATCHMAKING
-- ========================================

local function create_x4_match(context, payload)
    local user_id = context.user_id
    local username = context.username
    
    nk.logger_info(string.format("Creating X4 match for user %s", username))
    
    -- Create a new match
    local match_id = nk.match_create("x4_match", {})
    
    return nk.json_encode({
        success = true,
        match_id = match_id,
        message = "X4 match created successfully"
    })
end

nk.register_rpc(create_x4_match, "create_x4_match")

-- ========================================
-- SERVER INITIALIZATION
-- ========================================

-- Initialize server components
local function initialize_server()
    create_x4_leaderboards()
    nk.logger_info("X4 NakamaX4Client server initialization complete")
end

-- Run initialization
initialize_server()

nk.logger_info("X4 NakamaX4Client server runtime loaded successfully")