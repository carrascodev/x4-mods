#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <msgpack.hpp>
#include "../public/sector_match.h"

// Define the function signature for luaopen_test_script_x4
typedef int (*LuaOpenFunction)(void* L);

// Define the function signature for nakama_init
typedef int (*NakamaInitFunc)(const char* host, int port, const char* server_key);

// Test MessagePack serialization/deserialization
void testMessagePack()
{
    std::cout << "\n=== Testing MessagePack Serialization ===" << std::endl;
    
    try
    {
        // Create test data using PositionUpdate struct
        PositionUpdate original;
        original.player_id = "player123";
        original.position = {100.0f, 200.0f, 300.0f};
        original.rotation = {0.1f, 0.2f, 0.3f};
        original.velocity = {1.0f, 2.0f, 3.0f};
        
        // Serialize using msgpack::sbuffer
        msgpack::sbuffer sbuf;
        msgpack::pack(sbuf, original);
        
        std::cout << "Serialized data size: " << sbuf.size() << " bytes" << std::endl;
        
        // Deserialize
        msgpack::object_handle oh = msgpack::unpack(sbuf.data(), sbuf.size());
        msgpack::object obj = oh.get();
        
        // Convert to PositionUpdate struct
        PositionUpdate deserialized;
        obj.convert(deserialized);
        
        // Verify data
        bool success = true;
        if (deserialized.player_id != original.player_id) { std::cout << "ERROR: player_id mismatch" << std::endl; success = false; }
        if (deserialized.position != original.position) { std::cout << "ERROR: position mismatch" << std::endl; success = false; }
        if (deserialized.rotation != original.rotation) { std::cout << "ERROR: rotation mismatch" << std::endl; success = false; }
        if (deserialized.velocity != original.velocity) { std::cout << "ERROR: velocity mismatch" << std::endl; success = false; }
        
        if (success)
        {
            std::cout << "SUCCESS: MessagePack serialization/deserialization works correctly!" << std::endl;
        }
        else
        {
            std::cout << "FAILURE: Data mismatch in MessagePack test" << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "ERROR: MessagePack test failed with exception: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "=== C++ Nakama DLL Debug Test ===" << std::endl;
    
    // Test MessagePack functionality first
    testMessagePack();
    
    // Step 1: Try to load nakama_x4.dll
    std::cout << "Step 1: Loading nakama_x4.dll..." << std::endl;
    HMODULE nakamaModule = LoadLibraryA("nakama_x4.dll");
    if (!nakamaModule) {
        DWORD error = GetLastError();
        std::cout << "ERROR: Failed to load nakama_x4.dll, error code: " << error << std::endl;
        
        // Common error codes
        switch (error) {
            case 126: std::cout << "  - The specified module could not be found (missing dependencies)" << std::endl; break;
            case 127: std::cout << "  - The specified procedure could not be found" << std::endl; break;
            case 193: std::cout << "  - Not a valid Win32 application (architecture mismatch)" << std::endl; break;
            default: std::cout << "  - Other error" << std::endl; break;
        }
        return 1;
    }
    std::cout << "SUCCESS: nakama_x4.dll loaded at address: " << nakamaModule << std::endl;
    
    // Step 2: Try to get the luaopen_test_script_x4 function
    std::cout << "\nStep 2: Finding luaopen_test_script_x4 function..." << std::endl;
    LuaOpenFunction luaopen_func = (LuaOpenFunction)GetProcAddress(nakamaModule, "luaopen_test_script_x4");
    if (!luaopen_func) {
        DWORD error = GetLastError();
        std::cout << "ERROR: Failed to find luaopen_test_script_x4 function, error code: " << error << std::endl;
        FreeLibrary(nakamaModule);
        return 1;
    }
    std::cout << "SUCCESS: luaopen_test_script_x4 function found at address: " << luaopen_func << std::endl;
    
    // Step 2.5: Try to get the nakama_init function
    std::cout << "\nStep 2.5: Finding nakama_init function..." << std::endl;
    NakamaInitFunc nakama_init_func = (NakamaInitFunc)GetProcAddress(nakamaModule, "nakama_init");
    if (!nakama_init_func) {
        DWORD error = GetLastError();
        std::cout << "ERROR: Failed to find nakama_init function, error code: " << error << std::endl;
        FreeLibrary(nakamaModule);
        return 1;
    }
    std::cout << "SUCCESS: nakama_init function found at address: " << nakama_init_func << std::endl;
    
    // Step 3: Try to load dependencies manually
    std::cout << "\nStep 3: Checking dependencies..." << std::endl;
    
    const char* dependencies[] = {
        "nakama-sdk.dll",
    };
    
    for (const char* dep : dependencies) {
        HMODULE depModule = LoadLibraryA(dep);
        if (depModule) {
            std::cout << "  ✓ " << dep << " loaded successfully" << std::endl;
            // Don't free these as they might be needed
        } else {
            DWORD error = GetLastError();
            std::cout << "  ✗ " << dep << " failed to load, error: " << error << std::endl;
        }
    }
    
    // Step 4: Try to call luaopen_test_script_x4 (this is where it probably crashes)
    std::cout << "\nStep 4: Attempting to call luaopen_test_script_x4..." << std::endl;
    std::cout << "WARNING: This might crash! If it does, we know the issue is in the function itself." << std::endl;
    
    __try {
        // We'll pass NULL as the Lua state since we're just testing if the function can be called
        std::cout << "Calling luaopen_test_script_x4(NULL)..." << std::endl;
        int result = luaopen_func(nullptr);
        std::cout << "SUCCESS: luaopen_test_script_x4 returned: " << result << std::endl;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DWORD exceptionCode = GetExceptionCode();
        std::cout << "CRASH: Exception caught! Exception code: 0x" << std::hex << exceptionCode << std::dec << std::endl;
        
        switch (exceptionCode) {
            case EXCEPTION_ACCESS_VIOLATION:
                std::cout << "  - Access violation (reading/writing invalid memory)" << std::endl;
                break;
            case EXCEPTION_STACK_OVERFLOW:
                std::cout << "  - Stack overflow" << std::endl;
                break;
            case EXCEPTION_ILLEGAL_INSTRUCTION:
                std::cout << "  - Illegal instruction" << std::endl;
                break;
            case EXCEPTION_INT_DIVIDE_BY_ZERO:
                std::cout << "  - Division by zero" << std::endl;
                break;
            default:
                std::cout << "  - Other exception" << std::endl;
                break;
        }
    }
    
    // Step 4.5: Try to call nakama_init (this is where the logging should work)
    std::cout << "\nStep 4.5: Attempting to call nakama_init..." << std::endl;
    std::cout << "This will test the logging functionality." << std::endl;
    
    __try {
        std::cout << "Calling nakama_init(\"127.0.0.1\", 7350, \"defaultkey\")..." << std::endl;
        int result = nakama_init_func("127.0.0.1", 7350, "defaultkey");
        std::cout << "SUCCESS: nakama_init returned: " << result << std::endl;
        std::cout << "Check DebugView or the log file for LogToX4 output." << std::endl;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DWORD exceptionCode = GetExceptionCode();
        std::cout << "CRASH: Exception in nakama_init! Exception code: 0x" << std::hex << exceptionCode << std::dec << std::endl;
        
        switch (exceptionCode) {
            case EXCEPTION_ACCESS_VIOLATION:
                std::cout << "  - Access violation (reading/writing invalid memory)" << std::endl;
                break;
            case EXCEPTION_STACK_OVERFLOW:
                std::cout << "  - Stack overflow" << std::endl;
                break;
            case EXCEPTION_ILLEGAL_INSTRUCTION:
                std::cout << "  - Illegal instruction" << std::endl;
                break;
            case EXCEPTION_INT_DIVIDE_BY_ZERO:
                std::cout << "  - Division by zero" << std::endl;
                break;
            default:
                std::cout << "  - Other exception" << std::endl;
                break;
        }
    }
    
    // Step 5: Try with a simple Lua state
    std::cout << "\nStep 5: Testing with actual Lua state..." << std::endl;
    
    // Try to load lua51_64.dll and create a Lua state
    HMODULE luaModule = LoadLibraryA("lua51_64.dll");
    if (luaModule) {
        std::cout << "Lua DLL loaded, trying to create Lua state..." << std::endl;
        
        // Get lua functions
        typedef void* (*lua_newstate_func)(void* alloc, void* ud);
        typedef void (*lua_close_func)(void* L);
        
        lua_newstate_func lua_newstate = (lua_newstate_func)GetProcAddress(luaModule, "lua_newstate");
        lua_close_func lua_close = (lua_close_func)GetProcAddress(luaModule, "lua_close");
        
        if (lua_newstate && lua_close) {
            std::cout << "Lua functions found, this would be the full test..." << std::endl;
            std::cout << "(Skipping actual Lua state creation for safety)" << std::endl;
        } else {
            std::cout << "Could not find Lua state functions" << std::endl;
        }
    }
    
    // Cleanup
    std::cout << "\nStep 6: Cleanup..." << std::endl;
    FreeLibrary(nakamaModule);
    std::cout << "nakama_x4.dll unloaded" << std::endl;
    
    std::cout << "\n=== Test completed ===" << std::endl;
    std::cout << "Press any key to continue..." << std::endl;
    std::cin.get();
    
    return 0;
}