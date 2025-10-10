#pragma once

#ifdef NAKAMA_X4_EXPORTS
#define NAKAMA_X4_API __declspec(dllexport)
#else
#define NAKAMA_X4_API __declspec(dllimport)
#endif

extern "C" {
    #include "lua.h" // Forward declaration for lua_State
    // Lua module entry point (standard Lua C module convention)
    NAKAMA_X4_API int luaopen_nakama_x4(lua_State* L);
    
    // Basic lifecycle functions
    NAKAMA_X4_API int nakama_init(const char* host, int port, const char* server_key);
    NAKAMA_X4_API void nakama_shutdown();
    
    // Authentication
    NAKAMA_X4_API int nakama_authenticate(const char* device_id, const char* username);
    NAKAMA_X4_API int nakama_is_authenticated();
    
    // Simple data sync
    NAKAMA_X4_API int nakama_sync_player_data(const char* player_name, long long credits, long long playtime);
    
    // Status and utilities
    NAKAMA_X4_API const char* nakama_get_last_error();
    NAKAMA_X4_API const char* nakama_get_status();
    
    // Simple leaderboard
    NAKAMA_X4_API int nakama_submit_score(const char* leaderboard_id, long long score);
}