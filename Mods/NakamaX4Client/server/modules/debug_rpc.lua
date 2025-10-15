-- Debug RPC Functions
-- Utilities for inspecting Nakama Lua runtime
local nk = require("nakama")

local M = {}

-- RPC: Inspect the nakama module
function M.inspect_nakama_module(context, payload)
    local result = {
        functions = {},
        module_type = type(nk)
    }
    
    -- Iterate through all keys in the nk module
    for key, value in pairs(nk) do
        table.insert(result.functions, {
            name = key,
            type = type(value)
        })
    end
    
    -- Sort by name
    table.sort(result.functions, function(a, b) 
        return a.name < b.name 
    end)
    
    return nk.json_encode(result)
end

-- RPC: Get detailed help for Nakama functions
function M.get_nakama_help(context, payload)
    local help = {
        -- Core functions
        logger = {
            "nk.logger_info(message, ...)",
            "nk.logger_warn(message, ...)",
            "nk.logger_error(message, ...)",
            "nk.logger_debug(message, ...)"
        },
        
        -- Match functions
        matches = {
            "nk.match_create(module_name, setupstate)",
            "nk.match_get(match_id)",
            "nk.match_list(limit, authoritative, label, min_size, max_size, query)",
            "nk.match_signal(match_id, data)"
        },
        
        -- RPC
        rpc = {
            "nk.register_rpc(func, id)",
            "nk.rpc(context, id, payload)"
        },
        
        -- Storage
        storage = {
            "nk.storage_read(storage_reads)",
            "nk.storage_write(storage_writes)",
            "nk.storage_delete(storage_deletes)",
            "nk.storage_list(user_id, collection, limit, cursor)"
        },
        
        -- Users
        users = {
            "nk.users_get_id(user_ids)",
            "nk.users_get_username(usernames)",
            "nk.users_ban(user_ids)",
            "nk.account_get_id(user_id)",
            "nk.account_update_id(user_id, metadata, username, display_name, ...)"
        },
        
        -- Utility
        utility = {
            "nk.json_encode(table)",
            "nk.json_decode(string)",
            "nk.uuid_v4()",
            "nk.time()",
            "nk.base64_encode(input)",
            "nk.base64_decode(input)",
            "nk.http_request(url, method, headers, content, timeout)"
        },
        
        -- Matchmaker
        matchmaker = {
            "nk.matchmaker_add(presences, query, min_count, max_count, string_properties, numeric_properties)"
        },
        
        -- Leaderboards
        leaderboards = {
            "nk.leaderboard_create(id, authoritative, sort_order, operator, reset_schedule, metadata)",
            "nk.leaderboard_delete(id)",
            "nk.leaderboard_records_list(id, owners, limit, cursor, expiry)"
        },
        
        -- Notifications
        notifications = {
            "nk.notification_send(user_id, subject, content, code, sender_id, persistent)",
            "nk.notifications_send(notifications)"
        }
    }
    
    return nk.json_encode(help)
end

return M
