@echo off
REM Advanced Qt6 DLL deployment for GoodRAW Release Build using windeployqt

setlocal

set QT_PATH=C:\Qt\6.9.2\msvc2022_64\bin
set BUILD_DIR=%~dp0build\windows-release\Release

echo ================================================
echo Advanced Qt6 Deployment for GoodRAW Release Build
echo ================================================

if not exist "%QT_PATH%\windeployqt.exe" (
    echo ERROR: windeployqt.exe not found at %QT_PATH%
    echo Please check your Qt installation
    pause
    exit /b 1
)

if not exist "%BUILD_DIR%\GoodRAW.exe" (
    echo ERROR: GoodRAW.exe not found at %BUILD_DIR%
    echo Please build the release version first using build-windows-release.bat
    pause
    exit /b 1
)

echo Using Qt's official deployment tool...
echo.

REM Use windeployqt to deploy all necessary Qt dependencies (release mode)
"%QT_PATH%\windeployqt.exe" --release --compiler-runtime --opengl "%BUILD_DIR%\GoodRAW.exe"

echo.
echo Deploying vcpkg dependencies...

REM Deploy vcpkg release DLLs
set VCPKG_RELEASE_BIN=%~dp0vcpkg_installed\x64-windows\bin
if exist "%VCPKG_RELEASE_BIN%" (
    echo Copying vcpkg release DLLs from %VCPKG_RELEASE_BIN%
    copy "%VCPKG_RELEASE_BIN%\*.dll" "%BUILD_DIR%\" >nul 2>&1
    if errorlevel 1 (
        echo Warning: Some vcpkg DLLs could not be copied
    ) else (
        echo vcpkg release DLLs copied successfully
    )
) else (
    echo Warning: vcpkg release bin directory not found
)

echo.
echo ================================================
echo Complete RELEASE deployment finished successfully!
echo Qt6 and vcpkg dependencies have been deployed.
echo You can now run: %BUILD_DIR%\GoodRAW.exe
echo ================================================

pause