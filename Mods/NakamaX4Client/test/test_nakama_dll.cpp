#include <iostream>
#include <windows.h>
#include "../include/nakama_x4_api.h"

// Function pointer types
typedef int (*nakama_init_func)(const char*, int, const char*);
typedef void (*nakama_shutdown_func)();
typedef int (*nakama_authenticate_func)(const char*, const char*);
typedef int (*nakama_is_authenticated_func)();
typedef int (*nakama_sync_player_data_func)(const char*, long long, long long);
typedef const char* (*nakama_get_last_error_func)();
typedef const char* (*nakama_get_status_func)();

int main() {
    std::cout << "Loading nakama_x4.dll..." << std::endl;
    
    // Load the DLL
    HMODULE hDll = LoadLibraryA("nakama_x4.dll");
    if (!hDll) {
        std::cerr << "Failed to load nakama_x4.dll. Error: " << GetLastError() << std::endl;
        return 1;
    }
    
    // Get function pointers
    nakama_init_func nakama_init = (nakama_init_func)GetProcAddress(hDll, "nakama_init");
    nakama_shutdown_func nakama_shutdown = (nakama_shutdown_func)GetProcAddress(hDll, "nakama_shutdown");
    nakama_authenticate_func nakama_authenticate = (nakama_authenticate_func)GetProcAddress(hDll, "nakama_authenticate");
    nakama_is_authenticated_func nakama_is_authenticated = (nakama_is_authenticated_func)GetProcAddress(hDll, "nakama_is_authenticated");
    nakama_sync_player_data_func nakama_sync_player_data = (nakama_sync_player_data_func)GetProcAddress(hDll, "nakama_sync_player_data");
    nakama_get_last_error_func nakama_get_last_error = (nakama_get_last_error_func)GetProcAddress(hDll, "nakama_get_last_error");
    nakama_get_status_func nakama_get_status = (nakama_get_status_func)GetProcAddress(hDll, "nakama_get_status");
    
    if (!nakama_init || !nakama_shutdown || !nakama_authenticate || !nakama_get_status) {
        std::cerr << "Failed to get function pointers from DLL" << std::endl;
        FreeLibrary(hDll);
        return 1;
    }
    
    std::cout << "DLL loaded successfully, all functions found!" << std::endl;
    
    // Test status check
    std::cout << "Initial status: " << nakama_get_status() << std::endl;
    
    // Test initialization
    std::cout << "Initializing Nakama..." << std::endl;
    int result = nakama_init("127.0.0.1", 7350, "defaultkey");
    if (result == 0) {
        std::cout << "Nakama initialized successfully!" << std::endl;
    } else {
        std::cout << "Nakama initialization failed with code: " << result << std::endl;
        std::cout << "Error: " << nakama_get_last_error() << std::endl;
    }
    
    std::cout << "Status after init: " << nakama_get_status() << std::endl;
    
    // Test authentication
    std::cout << "Testing authentication..." << std::endl;
    result = nakama_authenticate("test_device_123", "TestPlayer");
    if (result == 0) {
        std::cout << "Authentication successful!" << std::endl;
    } else {
        std::cout << "Authentication failed with code: " << result << std::endl;
        std::cout << "Error: " << nakama_get_last_error() << std::endl;
    }
    
    std::cout << "Authenticated: " << (nakama_is_authenticated() ? "Yes" : "No") << std::endl;
    
    // Test data sync
    std::cout << "Testing player data sync..." << std::endl;
    result = nakama_sync_player_data("TestPlayer", 50000, 3600);
    if (result == 0) {
        std::cout << "Player data sync successful!" << std::endl;
    } else {
        std::cout << "Player data sync failed with code: " << result << std::endl;
        std::cout << "Error: " << nakama_get_last_error() << std::endl;
    }
    
    // Cleanup
    std::cout << "Final status: " << nakama_get_status() << std::endl;
    std::cout << "Cleaning up..." << std::endl;
    nakama_shutdown();
    
    FreeLibrary(hDll);
    std::cout << "Test completed!" << std::endl;
    
    return 0;
}