cmake_minimum_required(VERSION 3.15)

# Set vcpkg toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

project(X4Mods VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type - default to Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Include CMake utilities
include(Mods/CMakeUtils.cmake)

# Set the path to the Lua wrapper generator script (cache variable so it's available in subdirectories)
set(LUA_WRAPPER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/Mods/HenMod.Commons/scripts/generate_lua_wrappers.py" CACHE FILEPATH "Path to the Lua wrapper generator script")

# Add common library
add_subdirectory(Mods/HenMod.Commons)

# Add specific mods
add_subdirectory(Mods/NakamaX4Client)