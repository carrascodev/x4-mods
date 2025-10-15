#include "../public/nakama_realtime_client.h"
#include "../public/log_to_x4.h"
#include "../public/sector_match.h"
#include <chrono>
#include <future>
#include <msgpack.hpp>
#include <sstream>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>

NakamaRealtimeClient::NakamaRealtimeClient()
    : X4ScriptSingleton("NakamaRealtimeClient"), m_connected(false) {
}

NakamaRealtimeClient::~NakamaRealtimeClient() { Shutdown(); }

bool NakamaRealtimeClient::Initialize(
    std::shared_ptr<Nakama::NSessionInterface> session,
    std::shared_ptr<Nakama::NClientInterface> client, std::function<void(bool)> callback) {
    if (IsInitialized()) {
        LogWarning("Realtime client already initialized");
        if (callback) callback(true);
        return true;
    }

    if (!session || !client) {
        LogError("Invalid session or client provided");
        if (callback) callback(false);
        return false;
    }

    m_session = session;
    m_client = client;

    LogInfo("Initializing realtime client...");

    try {
        m_rtClient = m_client->createRtClient();

        // Set listener for realtime events
        m_rtClient->setListener(this);

        // Connect asynchronously
        auto connectFuture = m_rtClient->connectAsync(m_session, true);

        SetInitialized(true);
        if (callback) callback(true);
        LogInfo("Realtime client initialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        LogError("Exception initializing realtime client: %s", e.what());
        if (callback) callback(false);
        return false;
    }
}

void NakamaRealtimeClient::Shutdown() {
    if (!IsInitialized()) {
        return;
    }

    LogInfo("Shutting down realtime client");

    if (m_rtClient && m_connected) {
        m_rtClient->disconnect();
    }

    m_rtClient.reset();
    m_session.reset();
    m_client.reset();
    m_currentMatchId.clear();
    m_connected = false;

    SetInitialized(false);
    LogInfo("Realtime client shutdown complete");
}

void NakamaRealtimeClient::Update(float deltaTime) {
    // Call base class Update
    X4ScriptBase::Update(deltaTime);

    // Tick the realtime client if connected
    if (m_rtClient) {
        m_rtClient->tick();
    }
}

bool NakamaRealtimeClient::IsConnected() const { return m_connected; }

bool NakamaRealtimeClient::JoinOrCreateMatch(const std::string& sectorName) {
    if (!IsInitialized() || !m_connected) {
        LogError("Realtime client not initialized or not connected");
        return false;
    }

    LogInfo("Looking for sector match: %s", sectorName.c_str());

    try {

        std::future<Nakama::NRpc> matchResponseFuture = m_client->rpcAsync(
            m_session,
            "get_sector_match_id", 
            "{\"sector\":\"" + sectorName + "\"}");
        auto matchResponse = matchResponseFuture.get();
        // Parse the response JSON to find existing matches
        auto json = nlohmann::json::parse(matchResponse.payload);
        std::string matchId = json.value("match_id", "");
        if (!matchId.empty()) {
            LogInfo("Joining match: %s", matchId.c_str());
            auto joinFuture = m_rtClient->joinMatchAsync(matchId, {{"sector", sectorName}});
            joinFuture.wait_for(std::chrono::seconds(5)); // Wait for join to complete
            m_currentMatchId = matchId;
            OnMatchJoined(matchId);
            return true;
        }
        return false;
    }
    catch (const std::exception& e) {
        LogError("Exception while joining/creating match for sector %s: %s", 
                sectorName.c_str(), e.what());
        return false;
    }
}

void NakamaRealtimeClient::SendPosition(const std::string& data) {
    if (!IsInitialized() || !m_connected || m_currentMatchId.empty()) {
        LogWarning("Cannot send position: not connected or not in match");
        return;
    }

    try {
        // Convert string data to NBytes
        Nakama::NBytes dataBytes(data.begin(), data.end());

        m_rtClient->sendMatchData(m_currentMatchId, 1, dataBytes);
        LogInfo("Position data sent to match %s", m_currentMatchId.c_str());
    }
    catch (const std::exception& e) {
        LogError("Exception sending position: %s", e.what());
    }
}

void NakamaRealtimeClient::LeaveMatch() {
    if (!IsInitialized() || !m_connected || m_currentMatchId.empty()) {
        LogWarning("Cannot leave match: not connected or not in match");
        return;
    }

    try {
        LogInfo("Leaving match: %s", m_currentMatchId.c_str());

        m_rtClient->leaveMatch(m_currentMatchId);
        OnMatchLeft();
        m_currentMatchId.clear();
    }
    catch (const std::exception& e) {
        LogError("Exception leaving match: %s", e.what());
    }
}

void NakamaRealtimeClient::OnRealtimeConnected() {
    LogInfo("Realtime connection established");
}

void NakamaRealtimeClient::OnRealtimeDisconnected() {
    LogInfo("Realtime connection lost");
    m_currentMatchId.clear();
}

// NRtClientListenerInterface implementations
void NakamaRealtimeClient::onConnect() {
    LogInfo("Realtime client connected");
    m_connected = true;
    OnRealtimeConnected();
}

void NakamaRealtimeClient::onDisconnect(
    const Nakama::NRtClientDisconnectInfo& info) {
    LogInfo("Realtime client disconnected: %s", info.reason.c_str());
    m_connected = false;
    OnRealtimeDisconnected();
}

void NakamaRealtimeClient::onError(const Nakama::NRtError& error) {
    LogError("Realtime client error: %s", error.message.c_str());
}

void NakamaRealtimeClient::OnMatchJoined(const std::string& matchId) {
    LogInfo("Joined match: %s", matchId.c_str());
}

void NakamaRealtimeClient::OnMatchLeft() {
    LogInfo("Left current match");
    m_currentMatchId.clear();
}

void NakamaRealtimeClient::onMatchData(const Nakama::NMatchData& matchData) {
    // Handle incoming match data (position updates from other players)
    if (matchData.opCode == 1) // Position data opcode
    {
        try {
            // Deserialize MessagePack data into PositionUpdate struct
            msgpack::object_handle oh =
                msgpack::unpack(reinterpret_cast<const char*>(matchData.data.data()),
                    matchData.data.size());
            msgpack::object obj = oh.get();

            // Convert to PositionUpdate struct
            PositionUpdate update;
            obj.convert(update);

            if (update.player_id == m_session->getUserId()) {
                // Ignore updates from self
                return;
            }

            // Update remote player
            auto* sectorManager = SectorMatchManager::GetInstance();
            if (sectorManager) {
                sectorManager->UpdateRemotePlayer(update.player_id, update.position,
                    update.rotation, update.velocity);
            }
        }
        catch (const std::exception& e) {
            LogError("Failed to deserialize match data: %s", e.what());
        }
    }
}

void NakamaRealtimeClient::onMatchPresence(
    const Nakama::NMatchPresenceEvent& matchPresence) {
    // Handle players joining/leaving the match
    auto* sectorManager = SectorMatchManager::GetInstance();
    if (!sectorManager)
        return;

    for (const auto& presence : matchPresence.joins) {
        LogInfo("Player joined match: %s", presence.userId.c_str());
        // When a player joins, they should send their current position
        // For now, just add them as a remote player
        PlayerShip remoteShip(presence.userId, "remote_ship", true);
        sectorManager->OnSectorJoined(sectorManager->GetCurrentSector(),
            remoteShip);
    }

    for (const auto& presence : matchPresence.leaves) {
        LogInfo("Player left match: %s", presence.userId.c_str());
        // Remove the player from sector manager
        sectorManager->RemovePlayer(presence.userId);
    }
}