#pragma once

#include <string>
#include <map>
#include <vector>
#include <chrono>
#include "nakama_realtime_client.h"
#include "player_ship.h"
#include "x4_script_base.h"
#include <msgpack.hpp>

// Position update data structure for MessagePack serialization
struct PositionUpdate
{
    std::string player_id;
    std::vector<float> position;
    std::vector<float> rotation;
    std::vector<float> velocity;

    MSGPACK_DEFINE(player_id, position, rotation, velocity);
};

class SectorMatchManager : public X4ScriptSingleton<SectorMatchManager>
{
public:
    friend class X4ScriptSingleton<SectorMatchManager>;

    // Initializes the SectorMatchManager with the local player ID.
    // @param localPlayerId: The ID of the local player.
    // @return true if initialization succeeds, false otherwise.
    bool Initialize(const std::string& localPlayerId);

    // LUA_EXPORT
    void ChangeSector(const std::string& newSector);
    void OnSectorJoined(const std::string& sector, const PlayerShip& playerShip);
    void Shutdown();

    // Update remote player position (called when receiving network updates)
    void UpdateRemotePlayer(const std::string& playerId,
        const std::vector<float>& position,
        const std::vector<float>& rotation,
        const std::vector<float>& velocity);

    // Get all players currently in the sector
    // LUA_EXPORT
    const std::map<std::string, PlayerShip>& GetPlayersInSector() const;

    // Get current sector
    // LUA_EXPORT
    const std::string& GetCurrentSector() const { return m_currentSector; }

    // Remove a player from the sector
    void RemovePlayer(const std::string& playerId);

    // Get interpolated position for smooth rendering
    std::vector<float> GetInterpolatedPosition(const std::string& playerId) const;

    // Send local player position to other players
    // LUA_EXPORT
    void SendLocalPosition(const std::vector<float>& position,
        const std::vector<float>& rotation,
        const std::vector<float>& velocity);

    // Configuration
    void SetInterpolationDelay(float delayMs);
    void SetMaxSnapshotAge(int ageMs);
    void SetCleanupInterval(int intervalMs);

    SectorMatchManager();
    ~SectorMatchManager();

private:
    void Update(float deltaTime) override;
    void CleanupStalePlayers();
    void OnSectorLeft(const std::string& sector);

    std::map<std::string, PlayerShip> m_playerShips; // Keyed by player ID
    std::string m_localPlayerId;
    std::string m_currentSector;

    // Snapshot interpolation settings
    float m_interpolationDelayMs;
    int m_maxSnapshotAgeMs;
    int m_cleanupIntervalMs;
    std::chrono::steady_clock::time_point m_lastCleanupTime;
};