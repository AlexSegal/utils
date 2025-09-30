@echo off
REM GoodRAW Windows Build Script
REM This script simplifies building GoodRAW on Windows

setlocal

set CMAKE_PATH="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"

echo ================================================
echo Building GoodRAW for Windows (Debug)
echo ================================================

if not exist %CMAKE_PATH% (
    echo ERROR: CMake not found at expected location
    echo Please install Visual Studio 2022 BuildTools or adjust CMAKE_PATH
    pause
    exit /b 1
)

echo Using CMake: %CMAKE_PATH%

echo.
echo Step 1: Configuring build...
%CMAKE_PATH% --preset windows-debug-vcpkg
if errorlevel 1 (
    echo ERROR: Configuration failed
    pause
    exit /b 1
)

echo.
echo Step 2: Building project...
%CMAKE_PATH% --build --preset windows-debug-vcpkg --config Debug
if errorlevel 1 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo Step 3: Deploying Qt6 dependencies...
call deploy-qt-advanced.bat

echo.
echo ================================================
echo Build completed successfully!
echo Executable is ready at: build\windows-debug\Debug\GoodRAW.exe
echo ================================================
pause