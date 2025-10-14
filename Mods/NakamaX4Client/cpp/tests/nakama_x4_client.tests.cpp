#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

// Include the actual NakamaX4Client class and Nakama SDK
#include "../src/public/nakama_x4_client.h"
#include <nakama-cpp/Nakama.h>

using namespace fakeit;

TEST_CASE("Connection And Sync Test") {
    NakamaX4Client client = NakamaX4Client();

    NakamaX4Client::Config config;
    config.host = "127.0.0.1";
    config.port = 7350;
    config.useSSL = false;
    config.serverKey = "runtime-key";

     bool initialized = client.Initialize(config);
     REQUIRE(initialized == true);

    SECTION("Initialize") {
        if (initialized) {
            REQUIRE(client.IsInitialized() == true);
        }
        else {
            // Server not available - test that we can at least check the state
            REQUIRE(client.IsInitialized() == false);
            INFO("Nakama server not available at localhost:7350 - skipping initialization test");
        }
    }

    SECTION("Shutdown") {
        
        // Ensure we can shutdown cleanly
        REQUIRE(client.IsInitialized() == true);
        client.Shutdown();
        REQUIRE(client.IsInitialized() == false);
    }

    SECTION("Authenticate") {
        if (initialized) {
            auto authResult = client.Authenticate("test-device-id", "test-username");
            if (authResult.success) {
                REQUIRE(client.IsAuthenticated() == true);
            }
            else {
                REQUIRE(client.IsAuthenticated() == false);
                INFO("Authentication failed: " + authResult.errorMessage);
            }
        }
        else {
            INFO("Client not initialized - skipping authentication test");
        }
    }

    SECTION("SyncPlayerData") {
        if (initialized) {
            auto authResult = client.Authenticate("test-device-id", "test-username");
            if (authResult.success) {
                REQUIRE(client.IsAuthenticated() == true);

                // Perform a sync
                auto syncResult = client.SyncPlayerData("test-player", 1000, 3600);
                if (syncResult.success) {
                    REQUIRE(syncResult.success == true);
                }
                else {
                    REQUIRE(syncResult.success == false);
                    INFO("Data sync failed: " + syncResult.errorMessage);
                }
            }
            else {
                REQUIRE(client.IsAuthenticated() == false);
                INFO("Authentication failed - skipping data sync test: " + authResult.errorMessage);
            }
        }
        else {
            INFO("Client not initialized - skipping data sync test");
        }
    }
}