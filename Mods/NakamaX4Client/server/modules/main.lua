-- Main Nakama Lua Runtime Module
-- Registers all RPCs and match handlers for X4 Nakama integration
local nk = require("nakama")
local sector_rpc = require("sector_rpc")
local debug_rpc = require("debug_rpc")

nk.logger_info("=== X4 Nakama Mod - Initializing Lua Runtime ===")

-- Register sector RPCs
nk.register_rpc(sector_rpc.get_sector_match_id, "get_sector_match_id")
nk.logger_info("✓ Registered RPC: get_sector_match_id")

nk.register_rpc(sector_rpc.list_sector_matches, "list_sector_matches")
nk.logger_info("✓ Registered RPC: list_sector_matches")

-- Register debug RPCs (useful for development)
nk.register_rpc(debug_rpc.inspect_nakama_module, "inspect_nakama_module")
nk.logger_info("✓ Registered RPC: inspect_nakama_module")

nk.register_rpc(debug_rpc.get_nakama_help, "get_nakama_help")
nk.logger_info("✓ Registered RPC: get_nakama_help")

nk.logger_info("=== X4 Nakama Mod - Initialization Complete ===")
nk.logger_info("Note: Match handler 'sector_match_handler' is auto-discovered by module name")

