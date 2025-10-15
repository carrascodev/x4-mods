#pragma once
#include "x4_script_base.h"
#include <nakama-cpp/Nakama.h>
#include <nakama-cpp/realtime/NRtClientListenerInterface.h>
#include <string>
#include <memory>
#include <atomic>

// Nakama Realtime Client class - handles realtime connections and matchmaking
class NakamaRealtimeClient : public X4ScriptSingleton<NakamaRealtimeClient>, public Nakama::NRtClientListenerInterface {
public:
    friend class X4ScriptSingleton<NakamaRealtimeClient>;

    bool Initialize(std::shared_ptr<Nakama::NSessionInterface> session, std::shared_ptr<Nakama::NClientInterface> client, std::function<void(bool)> callback = nullptr);
    void Shutdown();

    // LUA_EXPORT
    bool IsConnected() const;
    // LUA_EXPORT
    bool JoinOrCreateMatch(const std::string& matchId = "");
    // LUA_EXPORT
    void SendPosition(const std::string& data);
    // LUA_EXPORT
    void LeaveMatch();

    // NRtClientListenerInterface overrides
    void onConnect() override;
    void onDisconnect(const Nakama::NRtClientDisconnectInfo& info) override;
    void onError(const Nakama::NRtError& error) override;
    void onMatchData(const Nakama::NMatchData& matchData) override;
    void onMatchPresence(const Nakama::NMatchPresenceEvent& matchPresence) override;

    void Update(float deltaTime) override;

    NakamaRealtimeClient();
    ~NakamaRealtimeClient();

private:
    std::shared_ptr<Nakama::NRtClientInterface> m_rtClient;
    std::shared_ptr<Nakama::NSessionInterface> m_session;
    std::shared_ptr<Nakama::NClientInterface> m_client;
    std::string m_currentMatchId;
    std::atomic<bool> m_connected;

    void OnRealtimeConnected();
    void OnRealtimeDisconnected();
    void OnMatchJoined(const std::string& matchId);
    void OnMatchLeft();
};