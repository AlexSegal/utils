@echo off
REM GoodRAW Windows Release Build Script
REM This script builds GoodRAW for Windows in Release mode

setlocal

set CMAKE_PATH="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"

echo ================================================
echo Building GoodRAW for Windows (Release)
echo ================================================

if not exist %CMAKE_PATH% (
    echo ERROR: CMake not found at expected location
    echo Please install Visual Studio 2022 BuildTools or adjust CMAKE_PATH
    pause
    exit /b 1
)

echo Using CMake: %CMAKE_PATH%

echo.
echo Step 1: Configuring release build...
%CMAKE_PATH% --preset windows-release-vcpkg
if errorlevel 1 (
    echo ERROR: Configuration failed
    pause
    exit /b 1
)

echo.
echo Step 2: Building release...
%CMAKE_PATH% --build --preset windows-release-vcpkg --config Release
if errorlevel 1 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ================================================
echo Release build completed successfully!
echo Output: build\windows-release\Release\GoodRAW.exe
echo ================================================

pause