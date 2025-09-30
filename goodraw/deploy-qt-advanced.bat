@echo off
REM Advanced Qt6 DLL deployment for GoodRAW using windeployqt

setlocal

set QT_PATH=C:\Qt\6.9.2\msvc2022_64\bin
set BUILD_DIR=%~dp0build\windows-debug\Debug

echo ================================================
echo Advanced Qt6 Deployment for GoodRAW Debug Build
echo ================================================

if not exist "%QT_PATH%\windeployqt.exe" (
    echo ERROR: windeployqt.exe not found at %QT_PATH%
    echo Falling back to manual deployment...
    call deploy-qt-debug.bat
    exit /b 0
)

if not exist "%BUILD_DIR%\GoodRAW.exe" (
    echo ERROR: GoodRAW.exe not found at %BUILD_DIR%
    echo Please build the project first
    pause
    exit /b 1
)

echo Using Qt's official deployment tool...
echo.

REM Use windeployqt to deploy all necessary Qt dependencies
"%QT_PATH%\windeployqt.exe" --debug --compiler-runtime --opengl "%BUILD_DIR%\GoodRAW.exe"

echo.
echo Deploying vcpkg dependencies...

REM Deploy vcpkg debug DLLs
set VCPKG_DEBUG_BIN=%~dp0vcpkg_installed\x64-windows\debug\bin
if exist "%VCPKG_DEBUG_BIN%" (
    echo Copying vcpkg debug DLLs from %VCPKG_DEBUG_BIN%
    copy "%VCPKG_DEBUG_BIN%\*.dll" "%BUILD_DIR%\" >nul 2>&1
    if errorlevel 1 (
        echo Warning: Some vcpkg DLLs could not be copied
    ) else (
        echo vcpkg debug DLLs copied successfully
    )
) else (
    echo Warning: vcpkg debug bin directory not found
)

echo.
echo ================================================
echo Complete deployment finished successfully!
echo Qt6 and vcpkg dependencies have been deployed.
echo You can now run: %BUILD_DIR%\GoodRAW.exe
echo ================================================

pause