cmake_minimum_required(VERSION 3.15)
set(CMAKE_TOOLCHAIN_FILE "D:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Lua configuration for X4
set(LUA_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/third-party/lua/lua51_64.lib")
set(LUA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third-party/lua/")

project(NakamaX4Client VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type - default to Debug for console flexibility
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Compiler-specific options
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building in Debug mode for console flexibility")
    if(MSVC)
        add_compile_options(/W4 /Zi /EHa)
    else()
        add_compile_options(-Wall -Wextra -g -O0)
    endif()
else()
    message(STATUS "Building in Release mode")
    if(MSVC)
        add_compile_options(/W4 /O2 /EHa)
    else()
        add_compile_options(-Wall -Wextra -O2)
    endif()
endif()

# Find required packages
find_package(Threads REQUIRED)

# Find nlohmann/json via vcpkg
find_package(nlohmann_json CONFIG REQUIRED)

# Platform-specific libraries
if(WIN32)
    set(PLATFORM_LIBS ws2_32)
else()
    set(PLATFORM_LIBS)
endif()

# Nakama SDK paths - use existing third-party location for now  
set(NAKAMA_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third-party/nakama-sdk/win-x64" CACHE PATH "Path to pre-compiled Nakama SDK")


# Check if Nakama SDK exists
if(EXISTS "${NAKAMA_SDK_ROOT}")
    message(STATUS "Found Nakama SDK at: ${NAKAMA_SDK_ROOT}")
    
    # Include directories
    set(NAKAMA_INCLUDE_DIR "${NAKAMA_SDK_ROOT}/include")
    set(NAKAMA_LIB_DIR "${NAKAMA_SDK_ROOT}/lib")
    set(NAKAMA_CMAKE_DIR "${NAKAMA_SDK_ROOT}/share/nakama-sdk")
    
    # Find the library (Windows uses .lib for linking, .dll for runtime)
    if(WIN32)
        find_library(NAKAMA_LIBRARY
            NAMES nakama-sdk
            PATHS "${NAKAMA_LIB_DIR}"
            NO_DEFAULT_PATH
        )
        
        # Also find the DLL for runtime
        find_file(NAKAMA_DLL
            NAMES nakama-sdk.dll
            PATHS "${NAKAMA_LIB_DIR}"
            NO_DEFAULT_PATH
        )
    else()
        find_library(NAKAMA_LIBRARY
            NAMES nakama-sdk libnakama-sdk
            PATHS "${NAKAMA_LIB_DIR}"
            NO_DEFAULT_PATH
        )
    endif()
    
    if(NAKAMA_LIBRARY)
        message(STATUS "Found Nakama library: ${NAKAMA_LIBRARY}")
        if(WIN32 AND NAKAMA_DLL)
            message(STATUS "Found Nakama DLL: ${NAKAMA_DLL}")
        endif()
        set(NAKAMA_FOUND TRUE)
    else()
        message(FATAL_ERROR "Could not find Nakama library in ${NAKAMA_LIB_DIR}")
    endif()
    
    # Include headers
    include_directories("${NAKAMA_INCLUDE_DIR}")
    
else()
    message(FATAL_ERROR "Nakama SDK not found at ${NAKAMA_SDK_ROOT}")
endif()

# Custom target to install missing dependencies via vcpkg
add_custom_target(install-deps
    COMMAND "${CMAKE_TOOLCHAIN_FILE}/../../vcpkg.exe" install nlohmann-json
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Installing missing dependencies via vcpkg"
    VERBATIM
)

# Debug target to show paths
add_custom_target(show-paths
    COMMAND ${CMAKE_COMMAND} -E echo "Nakama SDK Root: ${NAKAMA_SDK_ROOT}"
    COMMAND ${CMAKE_COMMAND} -E echo "Nakama Include: ${NAKAMA_INCLUDE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "Nakama Library: ${NAKAMA_LIBRARY}"
    COMMAND ${CMAKE_COMMAND} -E echo "Build Type: ${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} -E echo "nlohmann/json Found: ${nlohmann_json_FOUND}"
    COMMAND ${CMAKE_COMMAND} -E echo "vcpkg Toolchain: ${CMAKE_TOOLCHAIN_FILE}"
    VERBATIM
)

# Quick build help
add_custom_target(build-help
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Nakama Bridge Build Help:"
    COMMAND ${CMAKE_COMMAND} -E echo "========================"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Quick build (Debug for console debugging):"
    COMMAND ${CMAKE_COMMAND} -E echo "  mkdir build && cd build"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake .."
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build ."
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Install missing dependencies:"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build . --target install-deps"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Or manually: D:/dev/vcpkg/vcpkg.exe install nlohmann-json"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Release build:"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake -DCMAKE_BUILD_TYPE=Release .."
    COMMAND ${CMAKE_COMMAND} -E echo "  make -j4"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Run executable:"
    COMMAND ${CMAKE_COMMAND} -E echo "  ./NakamaBridge --host 127.0.0.1 --port 7350 --key defaultkey"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Show paths: cmake --build . --target show-paths"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    VERBATIM
)

# Nakama X4 DLL for Lua integration
add_library(nakama_x4 SHARED
    src/nakama_x4_dll.cpp
    src/LogToX4.cpp
)

target_include_directories(nakama_x4 PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${NAKAMA_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

if (MSVC)
target_link_options(nakama_x4 PRIVATE "/EXPORT:luaopen_nakama_x4")
endif()

target_link_libraries(nakama_x4 PRIVATE 
    ${NAKAMA_LIBRARY}
    Threads::Threads
    ${PLATFORM_LIBS}
    $<$<BOOL:${nlohmann_json_FOUND}>:nlohmann_json::nlohmann_json>
    ${LUA_LIBRARIES}
)

target_compile_definitions(nakama_x4 PRIVATE 
    NAKAMA_X4_EXPORTS
    HAVE_NAKAMA_SDK=1
)

# Set DLL output properties
set_target_properties(nakama_x4 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release"
)

# Copy Nakama DLL to output directory for the nakama_x4 DLL as well
if(WIN32 AND NAKAMA_DLL)
    add_custom_command(TARGET nakama_x4 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${NAKAMA_DLL}"
            "$<TARGET_FILE_DIR:nakama_x4>"
        COMMENT "Copying Nakama DLL for nakama_x4 DLL"
    )
endif()

# Debug executable to test DLL loading
add_executable(debug_nakama src/debug_nakama.cpp)
target_link_libraries(debug_nakama kernel32)