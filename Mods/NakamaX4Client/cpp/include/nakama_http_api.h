#pragma once

// Simple HTTP-based Nakama API without C++ SDK dependencies
#ifdef _WIN32
    #ifdef NAKAMA_X4_EXPORTS
        #define NAKAMA_X4_API __declspec(dllexport)
    #else
        #define NAKAMA_X4_API __declspec(dllimport)
    #endif
#else
    #define NAKAMA_X4_API
#endif

extern "C" {
    // Initialize HTTP-based Nakama client
    NAKAMA_X4_API int nakama_http_init(const char* host, int port, const char* server_key);
    
    // Shutdown and cleanup
    NAKAMA_X4_API void nakama_http_shutdown();
    
    // Authenticate with email/password via HTTP
    NAKAMA_X4_API int nakama_http_authenticate(const char* email, const char* password);
    
    // Check if authenticated
    NAKAMA_X4_API int nakama_http_is_authenticated();
    
    // Get account info via HTTP
    NAKAMA_X4_API int nakama_http_get_account(char* output_buffer, int buffer_size);
    
    // Get last error message
    NAKAMA_X4_API const char* nakama_http_get_last_error();
    
    // Get current status
    NAKAMA_X4_API const char* nakama_http_get_status();
}