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

TEST_CASE("NakamaX4Client Initialize/Shutdown", "[initialization]") {
    NakamaX4Client client;

    NakamaX4Client::Config config;
    config.host = "localhost";
    config.port = 7350;
    config.useSSL = false;
    config.serverKey = "defaultkey";
    
    // Note: This test will fail if Nakama server is not running
    // In a real test environment, you might want to mock the server
    bool initialized = client.Initialize(config);
    if (initialized) {
        REQUIRE(client.IsInitialized() == true);
        client.Shutdown();
        REQUIRE(client.IsInitialized() == false);
    } else {
        // Server not available - test that we can at least check the state
        REQUIRE(client.IsInitialized() == false);
        INFO("Nakama server not available at localhost:7350 - skipping initialization test");
    }
}

// Test basic functionality without complex dependencies
TEST_CASE("Basic test framework works", "[basic]") {
    REQUIRE(1 + 1 == 2);
    REQUIRE(std::string("hello") == "hello");
}