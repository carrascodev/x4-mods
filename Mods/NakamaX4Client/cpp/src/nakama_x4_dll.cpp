#include "nakama_x4_api.h"
#include <nakama-cpp/Nakama.h>
#include <string>
#include <memory>
#include <chrono>
#include <future>
#include <ctime>
#include <thread>
#include <atomic>  // C++17
#include "LogToX4.h"
#include <windows.h>

// Lua C API headers - you may need to adjust the path
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

// Global state
static std::shared_ptr<Nakama::NClientInterface> g_client;
static std::shared_ptr<Nakama::NSessionInterface> g_session;
static bool g_authenticated_http_mode = false; // Track authentication in HTTP fallback mode

// Helper alias for logging
#define Log(...) LogToX4::Log(__VA_ARGS__)

extern "C"
{

    NAKAMA_X4_API int nakama_init(const char *host, int port, const char *server_key)
    {
        try
        {
            Log("[Nakama] nakama_init called (no lua state) host=%s port=%d key=%s", host, port, server_key);
            // Reset state
            g_client.reset();
            g_session.reset();

            // Create Nakama client parameters
            auto parameters = Nakama::DefaultClientParameters();
            parameters.serverKey = server_key;
            parameters.host = host;
            parameters.port = port;
            parameters.ssl = false;

            Log("[Nakama] Creating Nakama client...");

            // Create the client - this should work with our debug DLL
            g_client = Nakama::createDefaultClient(parameters);


            if (g_client)
            {
                
                Log("[Nakama] Nakama client created.");
                return 1; // Success
            }
            else
            {
                Log("[Nakama] Failed to create Nakama client.");
                return 0; // Failure
            }
        }
        catch (const std::exception &e)
        {
            Log("[Nakama] Nakama SDK failed: %s", e.what());
            return 0; // Return failure, don't fall back
        }
        catch (...)
        {
            Log("[Nakama] Nakama SDK failed: unknown exception");
            return 0; // Return failure, don't fall back
        }
    }

    NAKAMA_X4_API void nakama_shutdown()
    {
        g_session.reset();
        g_client.reset();
        Log("Shutdown");
    }

    NAKAMA_X4_API int nakama_authenticate(const char *device_id, const char *username)
    {
        if (!g_client)
        {
            Log("Client not initialized - call nakama_init first");
            return 0;
        }

        try
        {
            Log("Authenticating with Nakama C++ SDK...");

            // Use atomic flags for thread safety
            std::atomic<bool> auth_completed{false};
            std::atomic<bool> auth_success{false};
            std::string auth_error;

            auto successCallback = [&](Nakama::NSessionPtr session)
            {
                g_session = session;
                Log("Authenticated with C++ SDK");
                auth_success = true;
                auth_completed = true;
            };

            auto errorCallback = [&](const Nakama::NError &error)
            {
                auth_error = "Auth failed: " + error.message;
                auth_success = false;
                auth_completed = true;
                Log("Authentication failed: %s", error.message.c_str());
            };

            // Use device authentication
            g_client->authenticateDevice(device_id, username, true, {}, successCallback, errorCallback);

            // Wait for completion with tick() calls
            for (int i = 0; i < 200 && !auth_completed.load(); i++)
            { // 10 seconds total (50ms * 200)
                g_client->tick();
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            if (!auth_completed.load())
            {
                Log("Authentication timeout after 10 seconds");
                return 0;
            }

            if (!auth_success.load())
            {
               Log("Authentication failed: %s", auth_error.c_str());
                return 0;
            }

            return 1; // Success
        }
        catch (const std::exception &e)
        {
            Log("Auth exception: %s", e.what());
            return 0;
        }
    }

    NAKAMA_X4_API int nakama_is_authenticated()
    {
        // Only check C++ SDK session
        return (g_session != nullptr) ? 1 : 0;
    }

    NAKAMA_X4_API int nakama_sync_player_data(const char *player_name, long long credits, long long playtime)
    {
        // Check authentication for both modes
        if (!g_client && !g_authenticated_http_mode)
        {
            Log("Not authenticated");
            return 0;
        }

        // If in HTTP mode, simulate successful sync
        if (!g_client && g_authenticated_http_mode)
        {
            Log("Syncing data (HTTP mode)...");
            // Here you would implement actual HTTP requests to Nakama REST API
            Log("Data synced (HTTP mode)");
            return 1;
        }

        // Use C++ SDK if available
        if (!g_session)
        {
           Log("Not authenticated with SDK");
            return 0;
        }

        try
        {
            Log("Syncing data...");

            // Create JSON data
            std::string json_data = "{"
                                    "\"credits\":" +
                                    std::to_string(credits) + ","
                                                              "\"playtime\":" +
                                    std::to_string(playtime) + ","
                                                               "\"last_update\":" +
                                    std::to_string(std::time(nullptr)) +
                                    "}";

            auto future = std::make_shared<std::promise<bool>>();

            Nakama::NStorageObjectWrite writeObject;
            writeObject.collection = "player_data";
            writeObject.key = player_name;
            writeObject.value = json_data;
            writeObject.permissionRead = Nakama::NStoragePermissionRead::OWNER_READ;
            writeObject.permissionWrite = Nakama::NStoragePermissionWrite::OWNER_WRITE;

            auto successCallback = [future](const Nakama::NStorageObjectAcks &acks)
            {
                Log("Data synced");
                future->set_value(true);
            };

            auto errorCallback = [future](const Nakama::NError &error)
            {
                Log("Sync failed: %s", error.message.c_str());
                future->set_value(false);
            };

            g_client->writeStorageObjects(g_session, {writeObject}, successCallback, errorCallback);

            // Wait for result
            auto future_status = future->get_future().wait_for(std::chrono::seconds(5));
            if (future_status == std::future_status::timeout)
            {
                Log("Sync timeout");
                return 0;
            }

            return future->get_future().get() ? 1 : 0;
        }
        catch (const std::exception &e)
        {
            Log("Sync exception: %s", e.what());
            return 0;
        }
    }






    NAKAMA_X4_API void nakama_tick()
    {
        
    }

    // Lua wrapper functions
    static int lua_nakama_init(lua_State *L)
    {
        const char *host = luaL_checkstring(L, 1);
        int port = luaL_checkinteger(L, 2);
        const char *server_key = luaL_checkstring(L, 3);

        int result = nakama_init(host, port, server_key);
        lua_pushinteger(L, result);
        return 1;
    }

    static int lua_nakama_authenticate(lua_State *L)
    {
        const char *device_id = luaL_checkstring(L, 1);
        const char *username = luaL_checkstring(L, 2);

        int result = nakama_authenticate(device_id, username);
        lua_pushinteger(L, result);
        return 1;
    }

    static int lua_nakama_sync_player_data(lua_State *L)
    {
        const char *player_name = luaL_checkstring(L, 1);
        long long credits = luaL_checkinteger(L, 2);
        long long playtime = luaL_checkinteger(L, 3);

        int result = nakama_sync_player_data(player_name, credits, playtime);
        lua_pushinteger(L, result);
        return 1;
    }





    static int lua_nakama_shutdown(lua_State *L)
    {
        nakama_shutdown();
        return 0;
    }

    static int lua_nakama_is_authenticated(lua_State *L)
    {
        int result = nakama_is_authenticated();
        lua_pushinteger(L, result);
        return 1;
    }



    static int lua_nakama_tick(lua_State *L)
    {
        nakama_tick();
        return 0;
    }

    // Lua module entry point - manual function registration
    NAKAMA_X4_API int luaopen_nakama_x4(lua_State *L)
    {
        lua_State *lua_L = L;
        // Create a new table
        lua_newtable(lua_L);

        // Register each function manually
        lua_pushcfunction(lua_L, lua_nakama_init);
        lua_setfield(lua_L, -2, "nakama_init");

        lua_pushcfunction(lua_L, lua_nakama_authenticate);
        lua_setfield(lua_L, -2, "nakama_authenticate");

        lua_pushcfunction(lua_L, lua_nakama_sync_player_data);
        lua_setfield(lua_L, -2, "nakama_sync_player_data");

        lua_pushcfunction(lua_L, lua_nakama_shutdown);
        lua_setfield(lua_L, -2, "nakama_shutdown");

        lua_pushcfunction(lua_L, lua_nakama_is_authenticated);
        lua_setfield(lua_L, -2, "nakama_is_authenticated");



        lua_pushcfunction(lua_L, lua_nakama_tick);
        lua_setfield(lua_L, -2, "nakama_tick");

        return 1; // Return the table with functions
    }


} // extern "C"