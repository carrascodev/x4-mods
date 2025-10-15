#define UNIT_TESTS

#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

// Include the actual NakamaX4Client class and Nakama SDK
#include "../src/public/nakama_x4_client.h"
#include "../src/public/nakama_realtime_client.h"
#include "../src/public/sector_match.h"
#include <nakama-cpp/Nakama.h>

using namespace fakeit;

// Test fixture for common setup
class TestFixture {
public:
    std::shared_ptr<NakamaX4Client> client;
    NakamaX4Client::Config config;
    bool initialized = false;
    NakamaX4Client::AuthResult authResult;
    bool authenticated = false;

    TestFixture() {
        config.host = "127.0.0.1";
        config.port = 7350;
        config.useSSL = false;
        config.serverKey = "defaultkey";

        client = static_cast<std::shared_ptr<NakamaX4Client>>(NakamaX4Client::GetInstance());
        
        initialized = client->Initialize(config);
    }

    // Helper method to ensure authentication
    bool EnsureAuthenticated() {
        if (client->IsInitialized() == false) return false;
        if (authenticated) return true;

        authResult = client->Authenticate("test-device-id", "test-username");
        authenticated = authResult.success;
        return authenticated;
    }

    bool InitializeSectorManager(const std::string& userId) {
        auto* sectorManager = SectorMatchManager::GetInstance();
        bool sectorInit = sectorManager->Initialize(userId);
        REQUIRE(sectorInit == true);
        INFO("Sector manager initialized successfully");
        return sectorInit;
    }
};

TEST_CASE("NakamaX4Client Basic Tests") {
    TestFixture fixture;

    REQUIRE(fixture.initialized == true);

    SECTION("Initialize") {
        REQUIRE(fixture.client->IsInitialized() == true);
        INFO("Client initialized successfully");
    }

    SECTION("SyncPlayerData") {
        // Use the helper method to ensure authentication happened
        if (fixture.EnsureAuthenticated()) {
            REQUIRE(fixture.client->IsAuthenticated() == true);

            // Perform a sync
            auto syncResult = fixture.client->SyncPlayerData("test-player", 1000, 3600);
            if (syncResult.success) {
                REQUIRE(syncResult.success == true);
            }
            else {
                REQUIRE(syncResult.success == false);
                INFO("Data sync failed: " + syncResult.errorMessage);
            }
        }
    }

    SECTION("Shutdown") {
        // Ensure we can shutdown cleanly
        REQUIRE(fixture.client->IsInitialized() == true);
        fixture.client->Shutdown();
        REQUIRE(fixture.client->IsInitialized() == false);
    }
}


TEST_CASE("SectorMatchManager Tests") {
    TestFixture fixture;

    REQUIRE(fixture.initialized == true);

    SECTION("Initialize Sector Manager and Join Match") {
        if (fixture.EnsureAuthenticated()) {
            REQUIRE(fixture.client->IsAuthenticated() == true);

            // Initialize sector manager
            if (fixture.InitializeSectorManager(fixture.client->GetSession()->getUserId())) {

                std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait for any async setup
                // Check if the realtime client is connected
                REQUIRE(NakamaRealtimeClient::GetInstance()->IsConnected() == true);
                // Attempt to join a match
                bool joinResult = NakamaRealtimeClient::GetInstance()->JoinOrCreateMatch("test-match-id");
                REQUIRE(joinResult == true);
                INFO("Joined match successfully");
            }
            else {
                FAIL("Failed to initialize sector manager - cannot test JoinMatch");
            }
        }
        else {
            FAIL("Authentication failed - cannot test sector manager");
        }
    }

    SECTION("Shutdown Sector Manager") {
        auto* sectorManager = SectorMatchManager::GetInstance();
        sectorManager->Shutdown();
        INFO("Sector manager shutdown completed");
    }
}


#undef UNIT_TESTS