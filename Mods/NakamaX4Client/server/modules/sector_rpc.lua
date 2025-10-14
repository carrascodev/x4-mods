-- Sector RPC Functions
-- Provides RPC endpoints for sector match management
local nk = require("nakama")

local M = {}

-- RPC: Get or create a sector match ID
-- Payload: { "sector": "SectorName" }
-- Returns: { "match_id": "uuid" }
function M.get_sector_match_id(context, payload)
    -- Parse the payload
    local data = nk.json_decode(payload)
    
    if not data.sector then
        error("sector parameter is required")
    end
    
    local sector = data.sector
    local label = "sector:" .. sector
    
    nk.logger_info(string.format("RPC get_sector_match_id called for sector: %s", sector))
    
    -- Search for existing match with this sector label
    local min_size = nil
    local max_size = nil
    local limit = 1
    local authoritative = true
    local query = nil
    
    local matches = nk.match_list(limit, authoritative, label, min_size, max_size, query)
    
    if #matches > 0 then
        -- Found existing match
        local match = matches[1]
        nk.logger_info(string.format("Found existing match for sector %s: %s (players: %d)", 
            sector, match.match_id, match.size))
        
        return nk.json_encode({
            match_id = match.match_id,
            size = match.size,
            created = false
        })
    else
        -- No match found, create a new one
        nk.logger_info(string.format("No match found for sector %s, creating new match", sector))
        
        -- Create match with sector metadata
        local module_name = "sector_match_handler"
        local setupstate = {
            sector = sector
        }
        
        local match_id = nk.match_create(module_name, setupstate)
        
        nk.logger_info(string.format("Created new match for sector %s: %s", sector, match_id))
        
        return nk.json_encode({
            match_id = match_id,
            size = 0,
            created = true
        })
    end
end

-- RPC: List all active sector matches
-- Payload: {} (optional)
-- Returns: [ { "sector": "name", "match_id": "uuid", "players": 0 } ]
function M.list_sector_matches(context, payload)
    nk.logger_info("RPC list_sector_matches called")
    
    -- List all authoritative matches
    local limit = 100
    local authoritative = true
    local label = "sector:"  -- All matches with sector: prefix
    
    local matches = nk.match_list(limit, authoritative, label, nil, nil, nil)
    
    local result = {}
    for _, match in ipairs(matches) do
        -- Extract sector name from label
        local sector = string.gsub(match.label, "^sector:", "")
        
        table.insert(result, {
            sector = sector,
            match_id = match.match_id,
            players = match.size,
            label = match.label
        })
    end
    
    nk.logger_info(string.format("Found %d active sector matches", #result))
    
    return nk.json_encode(result)
end

return M
