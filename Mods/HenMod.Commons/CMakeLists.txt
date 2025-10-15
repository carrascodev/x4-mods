cmake_minimum_required(VERSION 3.15)

project(HenMod.Commons VERSION 1.0.0 LANGUAGES CXX)

# Find required packages
find_package(nlohmann_json CONFIG REQUIRED)
find_package(msgpack-c CONFIG REQUIRED)

# Common library
add_library(henmod_commons STATIC
    cpp/src/private/log_to_x4.cpp
)

target_include_directories(henmod_commons PUBLIC
    cpp/src/public
    includes
)

target_link_libraries(henmod_commons PUBLIC
    nlohmann_json::nlohmann_json
    msgpack-c
)

target_compile_definitions(henmod_commons PUBLIC
    MSGPACK_NO_BOOST
)

# Tests for commons
enable_testing()
find_package(Catch2 3 REQUIRED)
find_package(FakeIt CONFIG REQUIRED)

add_executable(commons_tests
    tests/test_commons.cpp
)

target_include_directories(commons_tests PRIVATE
    cpp/src/public
    includes
)

target_link_libraries(commons_tests PRIVATE
    henmod_commons
    Catch2::Catch2WithMain
    FakeIt::FakeIt-catch
)

include(Catch)
catch_discover_tests(commons_tests)