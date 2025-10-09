#include "nakama_x4_api.h"
#include <nakama-cpp/Nakama.h>
#include <string>
#include <memory>
#include <chrono>
#include <future>
#include <ctime>
#include <thread>
#include <atomic>

// Lua C API headers - you may need to adjust the path
extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
}

// Global state
static std::shared_ptr<Nakama::NClientInterface> g_client;
static std::shared_ptr<Nakama::NSessionInterface> g_session;
static std::string g_last_error;
static std::string g_status = "Not initialized";
static bool g_nakama_sdk_available = true; // Track if Nakama SDK works
static bool g_authenticated_http_mode = false; // Track authentication in HTTP fallback mode

// Helper to set error message
void set_error(const std::string& error) {
    g_last_error = error;
    g_status = "Error: " + error;
}

// Helper to set status
void set_status(const std::string& status) {
    g_status = status;
}

extern "C" {

NAKAMA_X4_API int nakama_init(const char* host, int port, const char* server_key) {
    try {
        // Reset state
        g_client.reset();
        g_session.reset();
        g_authenticated_http_mode = false;
        
        // Create Nakama client parameters
        auto parameters = Nakama::NClientParameters();
        parameters.serverKey = server_key;
        parameters.host = host;
        parameters.port = port;
        parameters.ssl = false;
        
        // Create the client - this should work with our debug DLL
        g_client = Nakama::createDefaultClient(parameters);
        
        if (g_client) {
            set_status("Initialized (Nakama C++ SDK)");
            g_last_error.clear();
            g_nakama_sdk_available = true;
            return 1; // Success
        } else {
            set_error("Failed to create Nakama client");
            g_nakama_sdk_available = false;
            return 0; // Failure
        }
    }
    catch (const std::exception& e) {
        set_error("Nakama SDK failed: " + std::string(e.what()));
        g_nakama_sdk_available = false;
        return 0; // Return failure, don't fall back
    }
    catch (...) {
        set_error("Unknown error in Nakama SDK initialization");
        g_nakama_sdk_available = false;
        return 0; // Return failure, don't fall back
    }
}

NAKAMA_X4_API void nakama_shutdown() {
    g_session.reset();
    g_client.reset();
    set_status("Shutdown");
}

NAKAMA_X4_API int nakama_authenticate(const char* device_id, const char* username) {
    if (!g_client) {
        set_error("Client not initialized - call nakama_init first");
        return 0;
    }
    
    try {
        set_status("Authenticating with Nakama C++ SDK...");
        
        // Use atomic flags for thread safety
        std::atomic<bool> auth_completed{false};
        std::atomic<bool> auth_success{false};
        std::string auth_error;
        
        auto successCallback = [&](Nakama::NSessionPtr session) {
            g_session = session;
            set_status("Authenticated with C++ SDK");
            auth_success = true;
            auth_completed = true;
        };
        
        auto errorCallback = [&](const Nakama::NError& error) {
            auth_error = "Auth failed: " + error.message;
            auth_success = false;
            auth_completed = true;
        };
        
        // Use device authentication
        g_client->authenticateDevice(device_id, username, true, {}, successCallback, errorCallback);
        
        // Wait for completion with tick() calls
        for (int i = 0; i < 200 && !auth_completed.load(); i++) { // 10 seconds total (50ms * 200)
            g_client->tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        if (!auth_completed.load()) {
            set_error("Authentication timeout after 10 seconds");
            return 0;
        }
        
        if (!auth_success.load()) {
            set_error(auth_error);
            return 0;
        }
        
        return 1; // Success
    }
    catch (const std::exception& e) {
        set_error("Auth exception: " + std::string(e.what()));
        return 0;
    }
}

NAKAMA_X4_API int nakama_is_authenticated() {
    // Only check C++ SDK session
    return (g_session != nullptr) ? 1 : 0;
}

NAKAMA_X4_API int nakama_sync_player_data(const char* player_name, long long credits, long long playtime) {
    // Check authentication for both modes
    if (!g_client && !g_authenticated_http_mode) {
        set_error("Not authenticated");
        return 0;
    }
    
    // If in HTTP mode, simulate successful sync
    if (!g_client && g_authenticated_http_mode) {
        set_status("Syncing data (HTTP mode)...");
        // Here you would implement actual HTTP requests to Nakama REST API
        set_status("Data synced (HTTP mode)");
        return 1;
    }
    
    // Use C++ SDK if available
    if (!g_session) {
        set_error("Not authenticated with SDK");
        return 0;
    }
    
    try {
        set_status("Syncing data...");
        
        // Create JSON data
        std::string json_data = "{"
            "\"credits\":" + std::to_string(credits) + ","
            "\"playtime\":" + std::to_string(playtime) + ","
            "\"last_update\":" + std::to_string(std::time(nullptr)) +
            "}";
        
        auto future = std::make_shared<std::promise<bool>>();
        
        Nakama::NStorageObjectWrite writeObject{
            .collection = "player_data",
            .key = player_name,
            .value = json_data,
            .permissionRead = Nakama::NStoragePermissionRead::OWNER_READ,
            .permissionWrite = Nakama::NStoragePermissionWrite::OWNER_WRITE
        };
        
        auto successCallback = [future](const Nakama::NStorageObjectAcks& acks) {
            set_status("Data synced");
            future->set_value(true);
        };
        
        auto errorCallback = [future](const Nakama::NError& error) {
            set_error("Sync failed: " + error.message);
            future->set_value(false);
        };
        
        g_client->writeStorageObjects(g_session, {writeObject}, successCallback, errorCallback);
        
        // Wait for result
        auto future_status = future->get_future().wait_for(std::chrono::seconds(5));
        if (future_status == std::future_status::timeout) {
            set_error("Sync timeout");
            return 0;
        }
        
        return future->get_future().get() ? 1 : 0;
    }
    catch (const std::exception& e) {
        set_error("Sync exception: " + std::string(e.what()));
        return 0;
    }
}

NAKAMA_X4_API int nakama_submit_score(const char* leaderboard_id, long long score) {
    if (!g_client || !g_session) {
        set_error("Not authenticated");
        return 0;
    }
    
    try {
        set_status("Submitting score...");
        
        auto future = std::make_shared<std::promise<bool>>();
        
        auto successCallback = [future](const Nakama::NLeaderboardRecord& record) {
            set_status("Score submitted");
            future->set_value(true);
        };
        
        auto errorCallback = [future](const Nakama::NError& error) {
            set_error("Score submit failed: " + error.message);
            future->set_value(false);
        };
        
        g_client->writeLeaderboardRecord(g_session, leaderboard_id, score, 
                                       std::time(nullptr), {}, successCallback, errorCallback);
        
        // Wait for result
        auto future_status = future->get_future().wait_for(std::chrono::seconds(5));
        if (future_status == std::future_status::timeout) {
            set_error("Submit timeout");
            return 0;
        }
        
        return future->get_future().get() ? 1 : 0;
    }
    catch (const std::exception& e) {
        set_error("Submit exception: " + std::string(e.what()));
        return 0;
    }
}

NAKAMA_X4_API const char* nakama_get_last_error() {
    return g_last_error.c_str();
}

NAKAMA_X4_API const char* nakama_get_status() {
    return g_status.c_str();
}

NAKAMA_X4_API void nakama_tick() {
    if (g_client) {
        g_client->tick();
    }
}

// Lua wrapper functions
static int lua_nakama_init(lua_State* L) {
    const char* host = luaL_checkstring(L, 1);
    int port = luaL_checkinteger(L, 2);
    const char* server_key = luaL_checkstring(L, 3);
    
    int result = nakama_init(host, port, server_key);
    lua_pushinteger(L, result);
    return 1;
}

static int lua_nakama_authenticate(lua_State* L) {
    const char* device_id = luaL_checkstring(L, 1);
    const char* username = luaL_checkstring(L, 2);
    
    int result = nakama_authenticate(device_id, username);
    lua_pushinteger(L, result);
    return 1;
}

static int lua_nakama_sync_player_data(lua_State* L) {
    const char* player_name = luaL_checkstring(L, 1);
    long long credits = luaL_checkinteger(L, 2);
    long long playtime = luaL_checkinteger(L, 3);
    
    int result = nakama_sync_player_data(player_name, credits, playtime);
    lua_pushinteger(L, result);
    return 1;
}

static int lua_nakama_get_last_error(lua_State* L) {
    const char* error = nakama_get_last_error();
    lua_pushstring(L, error);
    return 1;
}

static int lua_nakama_get_status(lua_State* L) {
    const char* status = nakama_get_status();
    lua_pushstring(L, status);
    return 1;
}

static int lua_nakama_shutdown(lua_State* L) {
    nakama_shutdown();
    return 0;
}

static int lua_nakama_is_authenticated(lua_State* L) {
    int result = nakama_is_authenticated();
    lua_pushinteger(L, result);
    return 1;
}

static int lua_nakama_submit_score(lua_State* L) {
    const char* leaderboard_id = luaL_checkstring(L, 1);
    long long score = luaL_checkinteger(L, 2);
    
    int result = nakama_submit_score(leaderboard_id, score);
    lua_pushinteger(L, result);
    return 1;
}

static int lua_nakama_tick(lua_State* L) {
    nakama_tick();
    return 0;
}

// Lua module entry point - manual function registration
 NAKAMA_X4_API int luaopen_nakama_x4(void* L) {
    lua_State* lua_L = (lua_State*)L;
    
    // Create a new table
    lua_newtable(lua_L);
    
    // Register each function manually
    lua_pushcfunction(lua_L, lua_nakama_init);
    lua_setfield(lua_L, -2, "nakama_init");
    
    lua_pushcfunction(lua_L, lua_nakama_authenticate);
    lua_setfield(lua_L, -2, "nakama_authenticate");
    
    lua_pushcfunction(lua_L, lua_nakama_sync_player_data);
    lua_setfield(lua_L, -2, "nakama_sync_player_data");
    
    lua_pushcfunction(lua_L, lua_nakama_get_last_error);
    lua_setfield(lua_L, -2, "nakama_get_last_error");
    
    lua_pushcfunction(lua_L, lua_nakama_get_status);
    lua_setfield(lua_L, -2, "nakama_get_status");
    
    lua_pushcfunction(lua_L, lua_nakama_shutdown);
    lua_setfield(lua_L, -2, "nakama_shutdown");
    
    lua_pushcfunction(lua_L, lua_nakama_is_authenticated);
    lua_setfield(lua_L, -2, "nakama_is_authenticated");
    
    lua_pushcfunction(lua_L, lua_nakama_submit_score);
    lua_setfield(lua_L, -2, "nakama_submit_score");
    
    lua_pushcfunction(lua_L, lua_nakama_tick);
    lua_setfield(lua_L, -2, "nakama_tick");
    
    return 1;  // Return the table with functions
}

} // extern "C"