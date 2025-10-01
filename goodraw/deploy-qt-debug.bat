@echo off
REM Deploy Qt6 DLLs for GoodRAW Windows Debug Build

setlocal

set QT_PATH=C:\Qt\6.9.2\msvc2022_64\bin
set BUILD_DIR=%~dp0build\windows-debug\Debug

echo ================================================
echo Deploying Qt6 DLLs for GoodRAW Debug Build
echo ================================================

if not exist "%QT_PATH%" (
    echo ERROR: Qt6 not found at %QT_PATH%
    echo Please adjust QT_PATH in this script
    pause
    exit /b 1
)

if not exist "%BUILD_DIR%\GoodRAW.exe" (
    echo ERROR: GoodRAW.exe not found at %BUILD_DIR%
    echo Please build the project first
    pause
    exit /b 1
)

echo Copying Qt6 Debug DLLs...

REM Core Qt6 DLLs (Debug versions)
copy "%QT_PATH%\Qt6Cored.dll" "%BUILD_DIR%\" >nul
copy "%QT_PATH%\Qt6Guid.dll" "%BUILD_DIR%\" >nul  
copy "%QT_PATH%\Qt6Widgetsd.dll" "%BUILD_DIR%\" >nul
copy "%QT_PATH%\Qt6OpenGLd.dll" "%BUILD_DIR%\" >nul
copy "%QT_PATH%\Qt6OpenGLWidgetsd.dll" "%BUILD_DIR%\" >nul

REM Qt Platform Plugins (essential for Qt applications)
if not exist "%BUILD_DIR%\platforms" mkdir "%BUILD_DIR%\platforms"
copy "%QT_PATH%\..\plugins\platforms\qwindowsd.dll" "%BUILD_DIR%\platforms\" >nul
copy "%QT_PATH%\..\plugins\platforms\qminimald.dll" "%BUILD_DIR%\platforms\" >nul

REM Additional runtime dependencies
if exist "%QT_PATH%\msvcp140.dll" copy "%QT_PATH%\msvcp140.dll" "%BUILD_DIR%\" >nul
if exist "%QT_PATH%\vcruntime140.dll" copy "%QT_PATH%\vcruntime140.dll" "%BUILD_DIR%\" >nul
if exist "%QT_PATH%\vcruntime140_1.dll" copy "%QT_PATH%\vcruntime140_1.dll" "%BUILD_DIR%\" >nul

echo.
echo Qt6 DLLs deployed successfully!
echo You can now run: %BUILD_DIR%\GoodRAW.exe
echo.

pause