cmake_minimum_required(VERSION 3.15)

# Set vcpkg toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

project(HenMods VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type - default to Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Compiler-specific options
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        add_compile_options(/W4 /Zi /EHa)
    else()
        add_compile_options(-Wall -Wextra -g -O0)
    endif()
else()
    if(MSVC)
        add_compile_options(/W4 /O2 /EHa)
    else()
        add_compile_options(-Wall -Wextra -O2)
    endif()
endif()

# Find common packages
find_package(Threads REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(msgpack-c CONFIG REQUIRED)
find_package(Catch2 3 REQUIRED)
find_package(FakeIt CONFIG REQUIRED)

# Platform-specific libraries
if(WIN32)
    set(PLATFORM_LIBS ws2_32)
else()
    set(PLATFORM_LIBS)
endif()

# Add subdirectories for each mod
add_subdirectory(HenMod.Commons)
add_subdirectory(NakamaX4Client)