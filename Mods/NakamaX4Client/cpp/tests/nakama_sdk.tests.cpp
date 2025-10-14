#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <exception>
#include <nakama-cpp/Nakama.h>

TEST_CASE("Minimal Nakama SDK Test - Isolate Bad Allocation") {
    SECTION("Create Default Client") {
        try {
            std::cout << "Creating Nakama client..." << std::endl;
            
            Nakama::NClientParameters parameters;
            parameters.serverKey = "runtime-key";
            parameters.host = "127.0.0.1";
            parameters.port = 7350;
            
            std::cout << "Parameters set, calling createDefaultClient..." << std::endl;
            auto client = Nakama::createDefaultClient(parameters);
            
            std::cout << "Client created successfully!" << std::endl;
            REQUIRE(client != nullptr);
        }
        catch (const std::bad_alloc& e) {
            std::cout << "Bad allocation error: " << e.what() << std::endl;
            FAIL("Bad allocation when creating Nakama client: " << e.what());
        }
        catch (const std::exception& e) {
            std::cout << "Standard exception: " << e.what() << std::endl;
            FAIL("Exception when creating Nakama client: " << e.what());
        }
        catch (...) {
            std::cout << "Unknown exception caught" << std::endl;
            FAIL("Unknown exception when creating Nakama client");
        }
    }
}