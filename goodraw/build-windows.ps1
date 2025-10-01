# GoodRAW Windows Build Script (PowerShell)
# This script simplifies building GoodRAW on Windows

$ErrorActionPreference = "Stop"

$CMAKE_PATH = "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"

Write-Host "================================================" -ForegroundColor Green
Write-Host "Building GoodRAW for Windows (Debug)" -ForegroundColor Green  
Write-Host "================================================" -ForegroundColor Green

if (!(Test-Path $CMAKE_PATH)) {
    Write-Host "ERROR: CMake not found at expected location" -ForegroundColor Red
    Write-Host "Please install Visual Studio 2022 BuildTools or adjust CMAKE_PATH" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host "Using CMake: $CMAKE_PATH" -ForegroundColor Cyan

try {
    Write-Host ""
    Write-Host "Step 1: Configuring build..." -ForegroundColor Yellow
    & $CMAKE_PATH --preset windows-debug-vcpkg
    
    Write-Host ""
    Write-Host "Step 2: Building project..." -ForegroundColor Yellow
    & $CMAKE_PATH --build --preset windows-debug-vcpkg --config Debug
    
    Write-Host ""
    Write-Host "================================================" -ForegroundColor Green
    Write-Host "Build completed successfully!" -ForegroundColor Green
    Write-Host "Executable should be in: build\windows-debug\Debug\GoodRAW.exe" -ForegroundColor Green
    Write-Host "================================================" -ForegroundColor Green
}
catch {
    Write-Host "ERROR: Build failed - $($_.Exception.Message)" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Read-Host "Press Enter to exit"