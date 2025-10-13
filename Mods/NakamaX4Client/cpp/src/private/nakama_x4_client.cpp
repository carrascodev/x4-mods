#include "../public/nakama_x4_client.h"
#include "../public/log_to_x4.h"
#include "../public/nakama_realtime_client.h"
#include "../public/sector_match.h"
#include "../public/x4_script_base.h"
#include <atomic>
#include <chrono>
#include <ctime>
#include <thread>

NakamaX4Client::NakamaX4Client()
    : X4ScriptSingleton("NakamaX4Client"), m_authenticating(false),
      m_syncing(false), m_lastUpdateTime(std::chrono::steady_clock::now()) {}

bool NakamaX4Client::Initialize(const Config &config) {
  if (IsInitialized()) {
    LogWarning("Already initialized");
    return true;
  }

  LogInfo("Initializing Nakama client (host=%s, port=%d)", config.host.c_str(),
          config.port);

  if (!CreateClient(config)) {
    LogError("Failed to create Nakama client");
    return false;
  }

  SetInitialized(true);
  LogInfo("Nakama client initialized successfully");
  return true;
}

void NakamaX4Client::Shutdown() {
  if (!IsInitialized()) {
    return;
  }

  LogInfo("Shutting down Nakama client");

  auto *realtimeClient = NakamaRealtimeClient::GetInstance();
  if (realtimeClient && realtimeClient->IsInitialized()) {
    realtimeClient->Shutdown();
  }

  m_session.reset();
  m_client.reset();

  m_authenticating = false;
  m_syncing = false;
  m_updaterThread.join();

  SetInitialized(false);
  LogInfo("Nakama client shutdown complete");
}

std::thread NakamaX4Client::StartUpdater() {
  return std::thread([this]() {
    while (m_client != nullptr) {

      auto currentTime = std::chrono::steady_clock::now();
      std::chrono::duration<float> deltaTime = currentTime - m_lastUpdateTime;
      m_lastUpdateTime = currentTime;

      m_client->tick();
      Update(deltaTime.count());
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
  });
}

void NakamaX4Client::Update(float deltaTime) {
  // Call base class Update to handle callbacks
  X4ScriptBase::Update(deltaTime);

  // Update realtime client
  auto *realtimeClient = NakamaRealtimeClient::GetInstance();
  if (realtimeClient) {
    realtimeClient->Update(deltaTime);
  }

  // Process any pending async operations
  // This could be expanded to handle timeouts, retries, etc.
}

bool NakamaX4Client::CreateClient(const Config &config) {
  try {
    // Reset state
    m_client.reset();
    m_session.reset();

    // Create Nakama client parameters
    auto parameters = Nakama::DefaultClientParameters();
    parameters.serverKey = config.serverKey;
    parameters.host = config.host;
    parameters.port = config.port;
    parameters.ssl = config.useSSL;

    LogInfo("Creating Nakama client...");

    // Create the client
    m_client = Nakama::createDefaultClient(parameters);

    if (m_client) {
      LogInfo("Nakama client created successfully");
      m_updaterThread = StartUpdater();
      m_updaterThread.detach();
      return true;
    } else {
      LogError("Failed to create Nakama client");
      return false;
    }
  } catch (const std::exception &e) {
    LogError("Exception creating Nakama client: %s", e.what());
    return false;
  } catch (...) {
    LogError("Unknown exception creating Nakama client");
    return false;
  }
}

NakamaX4Client::AuthResult
NakamaX4Client::Authenticate(const std::string &deviceId,
                             const std::string &username) {
  if (!IsInitialized()) {
    return {false, "Client not initialized"};
  }

  if (m_authenticating) {
    return {false, "Authentication already in progress"};
  }

  LogInfo("Starting authentication (device=%s, username=%s)", deviceId.c_str(),
          username.c_str());
  return PerformAuthentication(deviceId, username);
}

NakamaX4Client::AuthResult
NakamaX4Client::PerformAuthentication(const std::string &deviceId,
                                      const std::string &username) {
  if (!m_client) {
    return {false, "Client not available"};
  }

  m_authenticating = true;

  try {
    LogInfo("Authenticating with Nakama...");

    auto future = std::make_shared<std::promise<AuthResult>>();

    auto successCallback = [this, future](Nakama::NSessionPtr session) {
      LogInfo("Authentication successful - Session created: %s",
              session ? "YES" : "NO");
      if (session) {
        LogInfo("Session user ID: %s", session->getUserId().c_str());
        LogInfo("Session username: %s", session->getUsername().c_str());
        LogInfo("Session token: %s",
                session->getAuthToken().substr(0, 20).c_str());
      }
      m_session = session;
      m_authenticating = false;

      LogInfo("Initializing realtime client...");
      auto *realtimeClient = NakamaRealtimeClient::GetInstance();
      if (realtimeClient->Initialize(m_session, m_client)) {
        LogInfo("Realtime client initialized successfully");

        // Initialize sector manager with local player ID
        auto *sectorManager = SectorMatchManager::GetInstance();
        if (sectorManager->Initialize(session->getUserId())) {
          LogInfo("Sector manager initialized successfully");
        } else {
          LogWarning("Failed to initialize sector manager");
        }
      } else {
        LogWarning("Failed to initialize realtime client");
      }

      future->set_value({true, ""});
    };

    auto errorCallback = [this, future](const Nakama::NError &error) {
      LogError("Authentication failed: %s (code: %d)", error.message.c_str(),
               error.code);
      m_authenticating = false;
      future->set_value({false, error.message});
    };

    // Use device authentication

    m_client->authenticateDevice(deviceId, username, true, {}, successCallback,
                                 errorCallback);

    auto fut = future->get_future();
    auto future_status = fut.wait_for(std::chrono::seconds(20));

    m_authenticating = false;
    return fut.get();
  } catch (const std::exception &e) {
    LogError("Authentication exception: %s", e.what());
    m_authenticating = false;
    return {false, std::string("Exception: ") + e.what()};
  }
}

NakamaX4Client::SyncResult
NakamaX4Client::SyncPlayerData(const std::string &playerName, long long credits,
                               long long playtime) {
  if (!IsInitialized()) {
    return {false, "Client not initialized"};
  }

  if (m_syncing) {
    return {false, "Sync already in progress"};
  }

  if (!m_session) {
    return {false, "Not authenticated"};
  }

  LogInfo("Starting data sync for player: %s", playerName.c_str());
  return PerformDataSync(playerName, credits, playtime);
}

NakamaX4Client::SyncResult
NakamaX4Client::PerformDataSync(const std::string &playerName,
                                long long credits, long long playtime) {
  m_syncing = true;

  try {
    LogInfo("Syncing player data...");

    // Create JSON data
    std::string json_data = "{"
                            "\"credits\":" +
                            std::to_string(credits) +
                            ","
                            "\"playtime\":" +
                            std::to_string(playtime) +
                            ","
                            "\"last_update\":" +
                            std::to_string(std::time(nullptr)) + "}";

    auto future = std::make_shared<std::promise<SyncResult>>();

    auto successCallback = [this,
                            future](const Nakama::NStorageObjectAcks &acks) {
      LogInfo("Data sync successful");
      m_syncing = false;
      future->set_value({true, ""});
    };

    auto errorCallback = [this, future](const Nakama::NError &error) {
      LogError("Data sync failed: %s", error.message.c_str());
      m_syncing = false;
      future->set_value({false, error.message});
    };

    Nakama::NStorageObjectWrite writeObject;
    writeObject.collection = "player_data";
    writeObject.key = playerName;
    writeObject.value = json_data;
    writeObject.permissionRead = Nakama::NStoragePermissionRead::OWNER_READ;
    writeObject.permissionWrite = Nakama::NStoragePermissionWrite::OWNER_WRITE;

    std::vector<Nakama::NStorageObjectWrite> objects = {writeObject};
    m_client->writeStorageObjects(m_session, objects, successCallback,
                                  errorCallback);

    // Wait for result with timeout
    auto fut = future->get_future();
    auto future_status = fut.wait_for(std::chrono::seconds(5));
    if (future_status == std::future_status::timeout) {
      LogError("Data sync timeout");
      m_syncing = false;
      return {false, "Sync timeout"};
    }

    return fut.get();
  } catch (const std::exception &e) {
    LogError("Data sync exception: %s", e.what());
    m_syncing = false;
    return {false, std::string("Exception: ") + e.what()};
  }
}

bool NakamaX4Client::IsAuthenticated() const { return m_session != nullptr; }