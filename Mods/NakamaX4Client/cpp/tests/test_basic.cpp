#include <catch2/catch_test_macros.hpp>

TEST_CASE("Basic test", "[basic]") {
    REQUIRE(1 + 1 == 2);
}

TEST_CASE("String test", "[string]") {
    std::string hello = "Hello";
    REQUIRE(hello == "Hello");
}