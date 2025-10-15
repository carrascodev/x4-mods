#include "../public/sector_match.h"
#include "../public/log_to_x4.h"
#include "../public/nakama_x4_client.h"
#include "../public/nakama_realtime_client.h"
#include "../public/player_ship.h"
#include <algorithm>
#include <chrono>
#include <msgpack.hpp>

// Magic numbers as named constants
constexpr float DEFAULT_INTERPOLATION_DELAY_MS = 100.0f;
constexpr int DEFAULT_MAX_SNAPSHOT_AGE_MS = 1000;
constexpr int DEFAULT_CLEANUP_INTERVAL_MS = 5000;

SectorMatchManager::SectorMatchManager()
	: X4ScriptSingleton("SectorMatchManager"), m_localPlayerId(""),
	m_currentSector(""), m_interpolationDelayMs(DEFAULT_INTERPOLATION_DELAY_MS),
	m_maxSnapshotAgeMs(DEFAULT_MAX_SNAPSHOT_AGE_MS), m_cleanupIntervalMs(DEFAULT_CLEANUP_INTERVAL_MS),
	m_lastCleanupTime(std::chrono::steady_clock::now()) {
}

SectorMatchManager::~SectorMatchManager() { Shutdown(); }

bool SectorMatchManager::Initialize(const std::string& localPlayerId)
{
	if (IsInitialized())
	{
		LogWarning("SectorMatchManager already initialized");
		return true;
	}

	m_localPlayerId = localPlayerId;

	// Register for realtime events if needed
	std::promise<bool> promise;
	std::future<bool> connectionResult = promise.get_future();

		auto* nakamaClient = NakamaX4Client::GetInstance();

		nakamaClient->ConnectRealtimeClient([&promise](bool success) {
			promise.set_value(success);
		});

		// Wait for the connection result (no timeout)
		bool success = connectionResult.get();
		
		if (success) {
			LogInfo("Connected to Nakama Realtime Client successfully");
			LogInfo("SectorMatchManager initialized for player: %s", m_localPlayerId.c_str());
			SetInitialized(true);
		}
		else {
			LogError("Failed to connect to Nakama Realtime Client");
			SetInitialized(false);
		}
		
		return success;
}

void SectorMatchManager::Shutdown()
{
	if (!IsInitialized())
	{
		return;
	}

	LogInfo("Shutting down SectorMatchManager");

	m_playerShips.clear();
	m_localPlayerId.clear();
	m_currentSector.clear();

	SetInitialized(false);
	LogInfo("SectorMatchManager shutdown complete");
}

void SectorMatchManager::ChangeSector(const std::string& newSector)
{
	if (!IsInitialized())
	{
		LogError("SectorMatchManager not initialized");
		return;
	}

	if (m_currentSector == newSector)
	{
		LogInfo("Already in sector: %s", newSector.c_str());
		return;
	}

	LogInfo("Changing sector from '%s' to '%s'", m_currentSector.c_str(),
		newSector.c_str());

	// Leave current sector and match
	if (!m_currentSector.empty())
	{
		OnSectorLeft(m_currentSector);
		// Leave the current match
		auto* rtClient = NakamaRealtimeClient::GetInstance();
		if (rtClient)
		{
			rtClient->LeaveMatch();
		}
	}

	// Clear all player ships for the new sector
	m_playerShips.clear();
	LogInfo("Cleared player ships map for new sector");

	// Join new sector
	m_currentSector = newSector;

	// Create local player ship
	PlayerShip localShip(m_localPlayerId, "local_ship", false);
	OnSectorJoined(newSector, localShip);

	// Join the new match for this sector
	auto* rtClient = NakamaRealtimeClient::GetInstance();
	if (rtClient && rtClient->IsConnected())
	{
		if (rtClient->JoinOrCreateMatch(newSector))
		{
			LogInfo("Joined match for sector %s",
				newSector.c_str());
		}
	}
}

void SectorMatchManager::OnSectorJoined(const std::string& sector,
	const PlayerShip& playerShip)
{
	if (sector != m_currentSector)
	{
		LogWarning("Received sector join for different sector: %s (current: %s)",
			sector.c_str(), m_currentSector.c_str());
		return;
	}

	// Add or update player ship
	m_playerShips[playerShip.player_id] = playerShip;
	LogInfo("Player %s joined sector %s", playerShip.player_id.c_str(),
		sector.c_str());
}

void SectorMatchManager::OnSectorLeft(const std::string& sector)
{
	if (sector != m_currentSector)
	{
		return;
	}

	LogInfo("Leaving sector: %s", sector.c_str());

	// Remove all remote players from this sector
	for (auto it = m_playerShips.begin(); it != m_playerShips.end();)
	{
		if (it->second.is_remote)
		{
			LogInfo("Removing remote player %s from sector", it->first.c_str());
			it = m_playerShips.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void SectorMatchManager::UpdateRemotePlayer(
	const std::string& playerId, const std::vector<float>& position,
	const std::vector<float>& rotation, const std::vector<float>& velocity)
{
	auto it = m_playerShips.find(playerId);
	if (it == m_playerShips.end())
	{
		// New remote player
		PlayerShip newShip(playerId, "", true);
		newShip.UpdatePosition(position, rotation, velocity);
		m_playerShips[playerId] = newShip;
		LogInfo("New remote player %s joined current sector", playerId.c_str());
	}
	else
	{
		// Update existing player
		it->second.UpdatePosition(position, rotation, velocity);
	}
}

const std::map<std::string, PlayerShip>& SectorMatchManager::GetPlayersInSector() const
{
	return m_playerShips;
}

std::vector<float>
SectorMatchManager::GetInterpolatedPosition(const std::string& playerId) const
{
	auto it = m_playerShips.find(playerId);

	if (it != m_playerShips.end())
	{
		return it->second.GetInterpolatedPosition(m_interpolationDelayMs);
	}
	return { 0.0f, 0.0f, 0.0f }; // Default position
}

void SectorMatchManager::RemovePlayer(const std::string& playerId)
{
	auto it = m_playerShips.find(playerId);
	if (it != m_playerShips.end())
	{
		LogInfo("Removing player %s from sector %s", playerId.c_str(),
			m_currentSector.c_str());
		m_playerShips.erase(it);
	}
}

void SectorMatchManager::SendLocalPosition(const std::vector<float>& position,
	const std::vector<float>& rotation,
	const std::vector<float>& velocity)
{
	if (!IsInitialized() || m_currentSector.empty())
	{
		return;
	}

	// Update local player ship
	auto it = m_playerShips.find(m_localPlayerId);
	if (it != m_playerShips.end())
	{
		it->second.UpdatePosition(position, rotation, velocity);
	}

	// Send to realtime client
	auto* rtClient = NakamaRealtimeClient::GetInstance();
	if (rtClient && rtClient->IsConnected())
	{
		// Create MessagePack message using PositionUpdate struct
		PositionUpdate update;
		update.player_id = m_localPlayerId;
		update.position = position;
		update.rotation = rotation;
		update.velocity = velocity;

		msgpack::sbuffer sbuf;
		msgpack::pack(sbuf, update);

		// Convert to string for SendPosition
		std::string msgpackStr(sbuf.data(), sbuf.size());
		rtClient->SendPosition(msgpackStr);
	}
}

void SectorMatchManager::Update(float deltaTime)
{
	if (!IsInitialized()) return;

	// Call base class Update
	X4ScriptBase::Update(deltaTime); // Lua callback should be sending the local player position

	// Periodic cleanup of stale players
	const auto now = std::chrono::steady_clock::now();
	if (now - m_lastCleanupTime >
		std::chrono::milliseconds(m_cleanupIntervalMs))
	{
		CleanupStalePlayers();
		m_lastCleanupTime = now;
	}
}

void SectorMatchManager::CleanupStalePlayers()
{
	const auto max_age = std::chrono::milliseconds(5000); // 5 seconds

	for (auto it = m_playerShips.begin(); it != m_playerShips.end();)
	{
		if (it->second.is_remote && it->second.IsStale(max_age))
		{
			LogInfo("Removing stale remote player: %s", it->first.c_str());
			it = m_playerShips.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void SectorMatchManager::SetInterpolationDelay(float delayMs)
{
	m_interpolationDelayMs = delayMs;
}

void SectorMatchManager::SetMaxSnapshotAge(int ageMs)
{
	m_maxSnapshotAgeMs = ageMs;
}

void SectorMatchManager::SetCleanupInterval(int intervalMs)
{
	m_cleanupIntervalMs = intervalMs;
}