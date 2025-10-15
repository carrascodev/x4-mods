#pragma once
#include "nakama_x4_client.h"
#include "player_ship.h"

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

// Helper to push PlayerShip to Lua
inline void PushPlayerShip(lua_State* L, const PlayerShip& ship) {
    lua_newtable(L);
    lua_pushstring(L, ship.player_id.c_str());
    lua_setfield(L, -2, "player_id");
    lua_pushstring(L, ship.ship_id.c_str());
    lua_setfield(L, -2, "ship_id");
    
    // Push position as table
    lua_newtable(L);
    for (size_t i = 0; i < ship.position.size(); ++i) {
        lua_pushnumber(L, ship.position[i]);
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "position");
    
    // Push rotation as table
    lua_newtable(L);
    for (size_t i = 0; i < ship.rotation.size(); ++i) {
        lua_pushnumber(L, ship.rotation[i]);
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "rotation");
    
    // Push velocity as table
    lua_newtable(L);
    for (size_t i = 0; i < ship.velocity.size(); ++i) {
        lua_pushnumber(L, ship.velocity[i]);
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "velocity");
    
    lua_pushboolean(L, ship.is_remote);
    lua_setfield(L, -2, "is_remote");
}

// Register a Lua callback with X4ScriptBase using the global Lua state
inline int RegisterLuaCallback(X4ScriptBase* script, int funcIndex) {
    lua_State* L = GetLuaState();
    if (!L) {
        LogToX4::Log("Error: Global Lua state not initialized");
        return -1;
    }
    
    if (!lua_isfunction(L, funcIndex)) {
        LogToX4::Log("Error: Expected Lua function at stack index %d", funcIndex);
        return -1;
    }
    
    // Duplicate the function on the stack
    lua_pushvalue(L, funcIndex);
    
    // Store it in the Lua registry and get a reference
    int luaFuncRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    // Register a C++ callback that will call the Lua function
    int callbackId = script->RegisterUpdateCallback([luaFuncRef](float deltaTime) {
        lua_State* L = GetLuaState();
        if (!L) return;
        
        // Push the Lua function onto the stack
        lua_rawgeti(L, LUA_REGISTRYINDEX, luaFuncRef);
        
        // Push deltaTime as argument
        lua_pushnumber(L, deltaTime);
        
        // Call the Lua function (1 arg, 0 returns, 0 error handler)
        if (lua_pcall(L, 1, 0, 0) != 0) {
            const char* error = lua_tostring(L, -1);
            LogToX4::Log("Error in Lua callback: %s", error);
            lua_pop(L, 1); // Remove error message
        }
    });
    
    return callbackId;
}

// Unregister Lua callbacks and clean up references
inline void UnregisterLuaCallback(X4ScriptBase* script, int callbackId) {
    if (callbackId >= 0) {
        script->UnregisterUpdateCallback(callbackId);
        // Note: We can't easily clean up the Lua registry reference here
        // without tracking which ref goes with which callback ID
        // Consider adding a map if cleanup is critical
    }
}