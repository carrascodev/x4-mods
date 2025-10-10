#pragma once
#include "x4_script_base.h"
#include <nakama-cpp/Nakama.h>
#include <string>
#include <memory>
#include <future>
#include <atomic>

// Nakama X4 Client class - encapsulates all Nakama functionality
class NakamaX4Client : public X4ScriptSingleton<NakamaX4Client> {
public:
    // Configuration structure
    struct Config {
        std::string host;
        int port;
        std::string serverKey;
        bool useSSL = false;
    };

    // Authentication result
    struct AuthResult {
        bool success;
        std::string errorMessage;
    };

    // Sync result
    struct SyncResult {
        bool success;
        std::string errorMessage;
    };

    friend class X4ScriptSingleton<NakamaX4Client>;

    // Public API methods
    // LUA_EXPORT
    bool Initialize(const Config& config);
    // LUA_EXPORT
    void Shutdown() override;
    // LUA_EXPORT
    AuthResult Authenticate(const std::string& deviceId, const std::string& username);
    // LUA_EXPORT
    SyncResult SyncPlayerData(const std::string& playerName, long long credits, long long playtime);
    // LUA_EXPORT
    bool IsAuthenticated() const;
    //
    void Tick(); // Call this regularly to process async operations
    
    // Status and error reporting
    const char* GetLastError() const;
    const char* GetStatus() const;

private:
public:
    NakamaX4Client();
    ~NakamaX4Client() override = default;

    // Nakama SDK objects
    std::shared_ptr<Nakama::NClientInterface> m_client;
    std::shared_ptr<Nakama::NSessionInterface> m_session;

    // Async operation handling
    std::atomic<bool> m_authenticating;
    std::atomic<bool> m_syncing;
    
    // Error and status tracking
    mutable std::string m_lastError;
    mutable std::string m_status;

    // Helper methods
    bool CreateClient(const Config& config);
    AuthResult PerformAuthentication(const std::string& deviceId, const std::string& username);
    SyncResult PerformDataSync(const std::string& playerName, long long credits, long long playtime);
    void Update(float deltaTime) override;
};
