project(NakamaX4Client VERSION 1.0.0 LANGUAGES CXX)

# Lua configuration for X4
set(LUA_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/../HenMod.Commons/includes/lua/lua51_64.lib")
set(LUA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../HenMod.Commons/includes/lua")

# Nakama SDK paths
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(NAKAMA_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/cpp/third-party/nakama-sdk-debug" CACHE PATH "Path to debug Nakama SDK")
else()
    set(NAKAMA_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/cpp/third-party/nakama-sdk/win-x64" CACHE PATH "Path to release Nakama SDK")
endif()

# Check if Nakama SDK exists
if(EXISTS "${NAKAMA_SDK_ROOT}")
    set(NAKAMA_INCLUDE_DIR "${NAKAMA_SDK_ROOT}/include")
    set(NAKAMA_LIB_DIR "${NAKAMA_SDK_ROOT}/lib")
    find_library(NAKAMA_LIBRARY NAMES nakama-sdk PATHS "${NAKAMA_LIB_DIR}")
    if(WIN32)
        find_file(NAKAMA_DLL NAMES nakama-sdk.dll PATHS "${NAKAMA_LIB_DIR}")
    endif()
endif()

# Generate Lua wrappers
set(GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src/generated")
set(HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src/public")

if(NOT EXISTS "${GENERATED_DIR}/generated_files.cmake")
    execute_process(
        COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/../HenMod.Commons/scripts/generate_lua_wrappers.py
            ${HEADER_DIR}
            --output_dir ${GENERATED_DIR}
    )
endif()
include(${GENERATED_DIR}/generated_files.cmake)

add_custom_target(generate_wrappers
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/../HenMod.Commons/scripts/generate_lua_wrappers.py
        ${HEADER_DIR}
        --output_dir ${GENERATED_DIR}
)

# Nakama X4 DLL
add_library(nakama_x4 SHARED
    cpp/src/private/nakama_x4_client.cpp
    cpp/src/private/nakama_realtime_client.cpp
    cpp/src/private/sector_match.cpp
    cpp/src/private/player_ship.cpp
    cpp/src/private/lua_bindings.cpp
    ${GENERATED_WRAPPERS}
)

target_include_directories(nakama_x4 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp/src/public
    ${NAKAMA_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../HenMod.Commons/cpp/src/public
)

target_link_libraries(nakama_x4 PRIVATE
    henmod_commons
    ${NAKAMA_LIBRARY}
    Threads::Threads
    ${PLATFORM_LIBS}
    ${LUA_LIBRARIES}
)

target_compile_definitions(nakama_x4 PRIVATE
    NAKAMA_X4_EXPORTS
    HAVE_NAKAMA_SDK=1
    MSGPACK_NO_BOOST
)

# Set output directory to ui/bin
set_target_properties(nakama_x4 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ui/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ui/bin"
)

# Copy Nakama DLL
if(WIN32 AND NAKAMA_DLL)
    add_custom_command(TARGET nakama_x4 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${NAKAMA_DLL}" "$<TARGET_FILE_DIR:nakama_x4>"
    )
endif()

# Tests
add_executable(nakama_tests
    cpp/tests/nakama_x4_client.tests.cpp
    cpp/tests/nakama_sdk.tests.cpp
    cpp/src/private/nakama_x4_client.cpp
    cpp/src/private/nakama_realtime_client.cpp
    cpp/src/private/sector_match.cpp
    cpp/src/private/player_ship.cpp
)

target_include_directories(nakama_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp/src/public
    ${NAKAMA_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../HenMod.Commons/cpp/src/public
)

target_link_libraries(nakama_tests
    henmod_commons
    Catch2::Catch2WithMain
    FakeIt::FakeIt-catch
    ${NAKAMA_LIBRARY}
    Threads::Threads
    ${PLATFORM_LIBS}
    ${LUA_LIBRARIES}
)

if(WIN32 AND NAKAMA_DLL)
    add_custom_command(TARGET nakama_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${NAKAMA_DLL}" "$<TARGET_FILE_DIR:nakama_tests>"
    )
endif()

include(Catch)
catch_discover_tests(nakama_tests)