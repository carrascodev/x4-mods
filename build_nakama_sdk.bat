@echo off
REM Build script for Nakama SDK with WebSocket and HTTP support using libHttpClient
REM This script builds the Nakama SDK from source with WebSocket capabilities enabled

echo Building Nakama C++ SDK with WebSocket support...
echo.

REM Get script directory and change to nakama-sdk
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%nakama-sdk"
if %ERRORLEVEL% NEQ 0 (
    echo Error: Could not navigate to nakama-sdk directory at %SCRIPT_DIR%nakama-sdk
    pause
    exit /b 1
)

echo Step 1: Configuring CMake with WebSocket and HTTP support...
cmake -S . -B build-with-ws ^
    -DCMAKE_TOOLCHAIN_FILE="submodules/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DWITH_WS_LIBHTTPC=ON ^
    -DWITH_HTTP_LIBHTTPC=ON

if %ERRORLEVEL% NEQ 0 (
    echo Error: CMake configuration failed
    pause
    exit /b 1
)

echo.
echo Step 2: Building Debug configuration...
cmake --build build-with-ws --config Debug

if %ERRORLEVEL% NEQ 0 (
    echo Error: Debug build failed
    pause
    exit /b 1
)

echo.
echo Step 3: Building MinSizeRel configuration (libHttpClient equivalent of Release)...
cmake --build build-with-ws --config MinSizeRel

if %ERRORLEVEL% NEQ 0 (
    echo Error: MinSizeRel build failed
    pause
    exit /b 1
)

echo.
echo Step 4: Copying built libraries to project...
cd /d "%SCRIPT_DIR%"

REM Copy Debug files
echo Copying Debug files...
copy "nakama-sdk\build-with-ws\Debug\nakama-sdk.dll" "Mods\NakamaX4Client\cpp\third-party\nakama-sdk\win-x64\lib\" /Y
copy "nakama-sdk\build-with-ws\Debug\nakama-sdk.lib" "Mods\NakamaX4Client\cpp\third-party\nakama-sdk\win-x64\lib\" /Y

REM Copy MinSizeRel files (equivalent to Release)
echo Copying MinSizeRel files...
copy "nakama-sdk\build-with-ws\MinSizeRel\nakama-sdk.dll" "Mods\NakamaX4Client\cpp\third-party\nakama-sdk\win-x64\lib\nakama-sdk-release.dll" /Y
copy "nakama-sdk\build-with-ws\MinSizeRel\nakama-sdk.lib" "Mods\NakamaX4Client\cpp\third-party\nakama-sdk\win-x64\lib\nakama-sdk-release.lib" /Y

REM Copy updated headers and config
echo Copying headers and config...
xcopy "nakama-sdk\interface\include\*" "Mods\NakamaX4Client\cpp\third-party\nakama-sdk\win-x64\include\" /E /H /C /I /Y
copy "nakama-sdk\build-with-ws\interface\nakama-cpp\config.h" "Mods\NakamaX4Client\cpp\third-party\nakama-sdk\win-x64\include\nakama-cpp\" /Y

REM Copy dependency DLLs to build directory
echo Copying dependency DLLs...
copy "nakama-sdk\build-with-ws\Debug\*.dll" "build\Debug\" /Y
copy "nakama-sdk\build-with-ws\MinSizeRel\*.dll" "build\Release\" /Y

echo.
echo ========================================
echo Nakama SDK build completed successfully!
echo ========================================
echo.
echo Built with features:
echo - WebSocket support (libHttpClient)
echo - HTTP support (libHttpClient)
echo - Debug and MinSizeRel configurations
echo.
echo Files copied to:
echo - Libraries: Mods\NakamaX4Client\cpp\third-party\nakama-sdk\win-x64\lib\
echo - Headers: Mods\NakamaX4Client\cpp\third-party\nakama-sdk\win-x64\include\
echo - Dependencies: build\Debug\ and build\Release\
echo.
pause