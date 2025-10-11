#pragma once
#include "nakama_x4_client.h"

// Lua C API headers
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

// Lua binding utilities and helper functions for NakamaX4Client

// Global Lua state for callbacks (set during DLL initialization)
extern lua_State* g_luaState;

// Set the Lua state for callback use
inline void SetLuaState(lua_State* L) {
    g_luaState = L;
}

// Get the Lua state for callbacks
inline lua_State* GetLuaState() {
    return g_luaState;
}

// Helper to push AuthResult to Lua
inline void PushAuthResult(lua_State* L, const NakamaX4Client::AuthResult& result) {
    lua_newtable(L);
    lua_pushboolean(L, result.success);
    lua_setfield(L, -2, "success");
    lua_pushstring(L, result.errorMessage.c_str());
    lua_setfield(L, -2, "error");
}

// Helper to push SyncResult to Lua
inline void PushSyncResult(lua_State* L, const NakamaX4Client::SyncResult& result) {
    lua_newtable(L);
    lua_pushboolean(L, result.success);
    lua_setfield(L, -2, "success");
    lua_pushstring(L, result.errorMessage.c_str());
    lua_setfield(L, -2, "error");
}