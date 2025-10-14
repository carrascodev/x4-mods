-- Sector Match Handler
-- Manages realtime multiplayer matches for X4 sectors
local nk = require("nakama")

local M = {}

-- Called when match is created
function M.match_init(context, setupstate)
    local sector = "unknown"
    
    -- Extract sector name from setupstate metadata
    if setupstate and setupstate.sector then
        sector = setupstate.sector
    end
    
    local state = {
        presences = {},
        sector = sector,
        label = "sector:" .. sector,
        tick_count = 0,
        created_at = nk.time()
    }
    
    local tick_rate = 10 -- 10 ticks per second (100ms)
    local label = state.label
    
    nk.logger_info(string.format("Match initialized for sector: %s", sector))
    
    return state, tick_rate, label
end

-- Called when a player attempts to join
function M.match_join_attempt(context, dispatcher, tick, state, presence, metadata)
    -- Allow anyone to join sector matches
    nk.logger_info(string.format("Player %s attempting to join sector %s", 
        presence.user_id, state.sector))
    
    return state, true, ""
end

-- Called when player(s) successfully join
function M.match_join(context, dispatcher, tick, state, presences)
    for _, presence in ipairs(presences) do
        state.presences[presence.session_id] = presence
        
        nk.logger_info(string.format("Player %s (username: %s) joined sector %s match. Total players: %d", 
            presence.user_id, 
            presence.username,
            state.sector,
            table_count(state.presences)))
        
        -- Send welcome message to the joining player
        local welcome_msg = string.format("Welcome to sector %s", state.sector)
        dispatcher.broadcast_message(1, welcome_msg, {presence})
    end
    
    return state
end

-- Called when player(s) leave
function M.match_leave(context, dispatcher, tick, state, presences)
    for _, presence in ipairs(presences) do
        state.presences[presence.session_id] = nil
        
        nk.logger_info(string.format("Player %s left sector %s match. Remaining players: %d", 
            presence.user_id,
            state.sector,
            table_count(state.presences)))
    end
    
    return state
end

-- Called every tick (10 times per second)
function M.match_loop(context, dispatcher, tick, state, messages)
    state.tick_count = state.tick_count + 1
    
    -- Process incoming messages (position updates, etc.)
    for _, message in ipairs(messages) do
        -- OpCode 1 = position update
        if message.op_code == 1 then
            -- Broadcast position update to all other players (not sender)
            dispatcher.broadcast_message(message.op_code, message.data, nil, message.sender)
        else
            -- Unknown message type - log it
            nk.logger_warn(string.format("Unknown message opcode %d from %s", 
                message.op_code, message.sender.user_id))
        end
    end
    
    -- Check if match is empty - terminate after grace period
    if table_count(state.presences) == 0 then
        if state.empty_since == nil then
            state.empty_since = tick
        elseif (tick - state.empty_since) > 600 then -- 60 seconds at 10 ticks/sec
            nk.logger_info(string.format("Terminating empty match for sector %s", state.sector))
            return nil  -- Terminate the match
        end
    else
        state.empty_since = nil
    end
    
    return state
end

-- Called when match is being terminated
function M.match_terminate(context, dispatcher, tick, state, grace_seconds)
    nk.logger_info(string.format("Match terminating for sector %s (ran for %d ticks)", 
        state.sector, state.tick_count))
    return state
end

-- Called when match receives a signal (external message)
function M.match_signal(context, dispatcher, tick, state, data)
    -- Handle external signals to the match (e.g., from nk.match_signal calls)
    nk.logger_info(string.format("Match signal received for sector %s: %s", 
        state.sector, data))
    return state, "signal_received"
end

-- Helper function to count table entries
function table_count(t)
    local count = 0
    for _ in pairs(t) do
        count = count + 1
    end
    return count
end

return M
