#!/bin/bash
# Build script for Nakama SDK with WebSocket and HTTP support using libHttpClient
# This script builds the Nakama SDK from source with WebSocket capabilities enabled

echo "Building Nakama C++ SDK with WebSocket support..."
echo ""

# Change to nakama-sdk directory
cd "$(dirname "$0")/nakama-sdk"
if [ $? -ne 0 ]; then
    echo "Error: Could not navigate to nakama-sdk directory"
    read -p "Press Enter to exit"
    exit 1
fi

echo "Step 1: Configuring CMake with WebSocket and HTTP support..."
cmake -S . -B build-with-ws \
    -DCMAKE_TOOLCHAIN_FILE="submodules/vcpkg/scripts/buildsystems/vcpkg.cmake" \
    -DWITH_WS_LIBHTTPC=ON \
    -DWITH_HTTP_LIBHTTPC=ON

if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed"
    read -p "Press Enter to exit"
    exit 1
fi

echo ""
echo "Step 2: Building Debug configuration..."
cmake --build build-with-ws --config Debug

if [ $? -ne 0 ]; then
    echo "Error: Debug build failed"
    read -p "Press Enter to exit"
    exit 1
fi

echo ""
echo "Step 3: Building MinSizeRel configuration (libHttpClient equivalent of Release)..."
cmake --build build-with-ws --config MinSizeRel

if [ $? -ne 0 ]; then
    echo "Error: MinSizeRel build failed"
    read -p "Press Enter to exit"
    exit 1
fi

echo ""
echo "Step 4: Copying built libraries to project..."
cd "$(dirname "$0")"

# Create directories if they don't exist
mkdir -p "Mods/NakamaX4Client/cpp/third-party/nakama-sdk/linux-amd64/lib"
mkdir -p "Mods/NakamaX4Client/cpp/third-party/nakama-sdk/linux-amd64/include"
mkdir -p "build/Debug"
mkdir -p "build/Release"

# Copy Debug files
echo "Copying Debug files..."
cp "nakama-sdk/build-with-ws/Debug/libnakama-sdk.so" "Mods/NakamaX4Client/cpp/third-party/nakama-sdk/linux-amd64/lib/" 2>/dev/null || \
cp "nakama-sdk/build-with-ws/libnakama-sdk.so" "Mods/NakamaX4Client/cpp/third-party/nakama-sdk/linux-amd64/lib/" 2>/dev/null || \
echo "Debug library not found (this may be normal for Linux builds)"

# Copy MinSizeRel files (equivalent to Release)
echo "Copying MinSizeRel files..."
cp "nakama-sdk/build-with-ws/MinSizeRel/libnakama-sdk.so" "Mods/NakamaX4Client/cpp/third-party/nakama-sdk/linux-amd64/lib/libnakama-sdk-release.so" 2>/dev/null || \
cp "nakama-sdk/build-with-ws/libnakama-sdk.so" "Mods/NakamaX4Client/cpp/third-party/nakama-sdk/linux-amd64/lib/libnakama-sdk-release.so" 2>/dev/null || \
echo "MinSizeRel library not found (this may be normal for Linux builds)"

# Copy updated headers and config
echo "Copying headers and config..."
cp -r "nakama-sdk/interface/include/"* "Mods/NakamaX4Client/cpp/third-party/nakama-sdk/linux-amd64/include/"
cp "nakama-sdk/build-with-ws/interface/nakama-cpp/config.h" "Mods/NakamaX4Client/cpp/third-party/nakama-sdk/linux-amd64/include/nakama-cpp/"

echo ""
echo "========================================"
echo "Nakama SDK build completed successfully!"
echo "========================================"
echo ""
echo "Built with features:"
echo "- WebSocket support (libHttpClient)"
echo "- HTTP support (libHttpClient)"
echo "- Debug and MinSizeRel configurations"
echo ""
echo "Files copied to:"
echo "- Libraries: Mods/NakamaX4Client/cpp/third-party/nakama-sdk/linux-amd64/lib/"
echo "- Headers: Mods/NakamaX4Client/cpp/third-party/nakama-sdk/linux-amd64/include/"
echo ""
read -p "Press Enter to exit"