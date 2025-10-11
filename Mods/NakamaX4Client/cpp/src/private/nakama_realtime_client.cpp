#include "../public/nakama_realtime_client.h"
#include "../public/log_to_x4.h"
#include <chrono>
#include <thread>

NakamaRealtimeClient::NakamaRealtimeClient()
    : X4ScriptSingleton("NakamaRealtimeClient"), m_connected(false)
{
}

NakamaRealtimeClient::~NakamaRealtimeClient()
{
    Shutdown();
}

bool NakamaRealtimeClient::Initialize(std::shared_ptr<Nakama::NSessionInterface> session, std::shared_ptr<Nakama::NClientInterface> client)
{
    if (IsInitialized())
    {
        LogWarning("Realtime client already initialized");
        return true;
    }

    if (!session || !client)
    {
        LogError("Invalid session or client provided");
        return false;
    }

    m_session = session;
    m_client = client;

    LogInfo("Initializing realtime client...");

    try
    {
        m_rtClient = m_client->createRtClient();

        // Set listener for realtime events
        m_rtClient->setListener(this);

        // Connect asynchronously
        auto connectFuture = m_rtClient->connectAsync(m_session, true);

        SetInitialized(true);
        LogInfo("Realtime client initialized successfully");
        return true;
    }
    catch (const std::exception& e)
    {
        LogError("Exception initializing realtime client: %s", e.what());
        return false;
    }
}

void NakamaRealtimeClient::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    LogInfo("Shutting down realtime client");

    if (m_rtClient && m_connected)
    {
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

void NakamaRealtimeClient::Update(float deltaTime)
{
    // Call base class Update
    X4ScriptBase::Update(deltaTime);

    // Tick the realtime client if connected
    if (m_rtClient)
    {
        m_rtClient->tick();
    }
}

bool NakamaRealtimeClient::IsConnected() const
{
    return m_connected;
}

bool NakamaRealtimeClient::JoinOrCreateMatch(const std::string& matchId)
{
    if (!IsInitialized() || !m_connected)
    {
        LogError("Realtime client not initialized or not connected");
        return false;
    }

    try
    {
        LogInfo("Joining or creating match: %s", matchId.empty() ? "auto" : matchId.c_str());

        auto successCallback = [this](const Nakama::NMatch& match) {
            LogInfo("Successfully joined match: %s", match.matchId.c_str());
            m_currentMatchId = match.matchId;
            OnMatchJoined(match.matchId);
        };

        auto errorCallback = [this](const Nakama::NRtError& error) {
            LogError("Failed to join/create match: %s", error.message.c_str());
        };

        if (matchId.empty())
        {
            // Create a new match
            m_rtClient->createMatch(successCallback, errorCallback);
        }
        else
        {
            // Join existing match
            m_rtClient->joinMatch(matchId, {}, successCallback, errorCallback);
        }

        return true;
    }
    catch (const std::exception& e)
    {
        LogError("Exception joining/creating match: %s", e.what());
        return false;
    }
}

void NakamaRealtimeClient::SendPosition(const std::string& data)
{
    if (!IsInitialized() || !m_connected || m_currentMatchId.empty())
    {
        LogWarning("Cannot send position: not connected or not in match");
        return;
    }

    try
    {
        // Convert string data to NBytes
        Nakama::NBytes dataBytes(data.begin(), data.end());

        m_rtClient->sendMatchData(m_currentMatchId, 1, dataBytes);
        LogInfo("Position data sent to match %s", m_currentMatchId.c_str());
    }
    catch (const std::exception& e)
    {
        LogError("Exception sending position: %s", e.what());
    }
}

void NakamaRealtimeClient::LeaveMatch()
{
    if (!IsInitialized() || !m_connected || m_currentMatchId.empty())
    {
        LogWarning("Cannot leave match: not connected or not in match");
        return;
    }

    try
    {
        LogInfo("Leaving match: %s", m_currentMatchId.c_str());

        m_rtClient->leaveMatch(m_currentMatchId);
        OnMatchLeft();
        m_currentMatchId.clear();
    }
    catch (const std::exception& e)
    {
        LogError("Exception leaving match: %s", e.what());
    }
}

void NakamaRealtimeClient::OnRealtimeConnected()
{
    LogInfo("Realtime connection established");
}

void NakamaRealtimeClient::OnRealtimeDisconnected()
{
    LogInfo("Realtime connection lost");
    m_currentMatchId.clear();
}

// NRtClientListenerInterface implementations
void NakamaRealtimeClient::onConnect()
{
    LogInfo("Realtime client connected");
    m_connected = true;
    OnRealtimeConnected();
}

void NakamaRealtimeClient::onDisconnect(const Nakama::NRtClientDisconnectInfo& info)
{
    LogInfo("Realtime client disconnected: %s", info.reason.c_str());
    m_connected = false;
    OnRealtimeDisconnected();
}

void NakamaRealtimeClient::onError(const Nakama::NRtError& error)
{
    LogError("Realtime client error: %s", error.message.c_str());
}

void NakamaRealtimeClient::OnMatchJoined(const std::string& matchId)
{
    LogInfo("Joined match: %s", matchId.c_str());
}

void NakamaRealtimeClient::OnMatchLeft()
{
    LogInfo("Left current match");
}